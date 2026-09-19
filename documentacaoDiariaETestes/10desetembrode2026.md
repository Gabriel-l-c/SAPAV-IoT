Arquitetura Edge-Gateway: LD2420 + ESP8266 + Raspberry Pi 4 + FIWARE Orion-LDEsta documentação detalha a evolução da arquitetura do projeto. O ESP8266 foi rebaixado a um Nó Sensor (Edge Node), responsável apenas pela leitura em tempo real e filtro básico. O Raspberry Pi 4 assume o papel de Gateway IoT, realizando a ponte USB-Serial, a formatação de dados para o padrão ETSI NGSI-LD e a comunicação de rede HTTP com o FIWARE Orion-LD.⚠️ Limitações e Comportamento do Sensor LD2420Antes de analisar o código, é vital compreender as limitações de hardware do radar mmWave:Falta de Reconhecimento Semântico: O sensor não sabe diferenciar um ser humano de outros objetos. Ele detecta qualquer reflexão de micro-ondas no ambiente.Falsos Positivos (Valores Fantasmas): O sensor pode acusar presença e distâncias aleatórias mesmo quando a sala está completamente vazia e sem movimento. Isso ocorre devido a ruídos de rádio, reflexão em móveis metálicos, paredes, ou interferência elétrica.Objetos Estáticos/Mecânicos: Elementos como ventiladores girando, cortinas ao vento ou até mesmo água correndo em canos podem ser interpretados como "movimento" ou "presença contínua".Nota de Engenharia: O filtro de variação de 5cm (LIMITE_MUDANCA_DISTANCIA) ajuda a ignorar parte do jitter estático, mas o posicionamento físico do sensor e a configuração das "portas" (gates) de sensibilidade no app oficial da Hi-Link são essenciais para reduzir os valores fantasmas.1. Topologia do SistemaPlaintext[ Sensor LD2420 ] 
       │ UART (38400 baud)
       ▼
[ ESP8266 NodeMCU ] (Nó Sensor Edge)
       │ - Lê dados seriais
       │ - Aplica filtro de zona morta (>= 5cm)
       │ - Imprime JSON simples (Serial/USB)
       ▼
[ Cabo USB ]
       ▼
[ Raspberry Pi 4 ] (Gateway IoT)
       │ - Ouve /dev/ttyUSB0 via Python (pyserial)
       │ - Converte JSON simples em Payload NGSI-LD
       │ - Adiciona @context, type Property e unitCode
       ▼
[ Rede Local / Ethernet ]
       ▼
[ FIWARE Orion-LD ] (Context Broker - <ip aqui>)
Pinagem LD2420 $\leftrightarrow$ ESP8266:TX do Sensor $\rightarrow$ Pino 13 (D7) do ESP8266 (RX da SoftwareSerial)RX do Sensor $\rightarrow$ Pino 15 (D8) do ESP8266 (TX da SoftwareSerial)VCC/GND $\rightarrow$ 5V ou 3.3V / GND2. Firmware do Nó Sensor (ESP8266)O código abaixo deve ser gravado no ESP8266. Ele foi purgado de lógicas de rede (Wi-Fi/HTTP), atuando como um conversor UART-para-USB super rápido.C++
#include <SoftwareSerial.h>
#include "LD2420.h"

#define RX_PIN 13 // Conectado ao TX do LD2420
#define TX_PIN 15 // Conectado ao RX do LD2420

SoftwareSerial sensorSerial(RX_PIN, TX_PIN);
LD2420 radar;

int lastDistance = -1;
bool wasDetecting = false;
unsigned long ultimoEnvio = 0;
const int tempoMinimoEntreEnvios = 500; 
const int LIMITE_MUDANCA_DISTANCIA = 5;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  sensorSerial.begin(115200);
  if (!radar.begin(sensorSerial)) {
    Serial.println("{\"error\": \"Falha na comunicacao com LD2420\"}");
    while (1) delay(1000);
  }

  radar.setDistanceRange(0, 600);
  radar.setUpdateInterval(50);
  
  Serial.println("{\"status\": \"Gateway Edge Node iniciado\"}");
}

// Emite JSON enxuto via USB para o Raspberry Pi
void enviarParaUSB(bool isDetecting, int distance) {
  Serial.printf("{\"presence\": %s, \"distance\": %d}\n", isDetecting ? "true" : "false", distance);
}

void loop() {
  radar.update();

  bool isDetecting = radar.isDetecting();
  int currentDistance = radar.getDistance();

  if (millis() - ultimoEnvio > tempoMinimoEntreEnvios) {
    if (isDetecting) {
      // Regra de transição ou variação de 5cm
      if (lastDistance == -1 || abs(currentDistance - lastDistance) >= LIMITE_MUDANCA_DISTANCIA) {
        enviarParaUSB(true, currentDistance);
        lastDistance = currentDistance;
        wasDetecting = true;
        ultimoEnvio = millis();
      }
    } else {
      // Regra de borda de descida (movimento cessado)
      if (wasDetecting) {
        enviarParaUSB(false, 0);
        lastDistance = -1;
        wasDetecting = false;
        ultimoEnvio = millis();
      }
    }
  }
  delay(10);
}
3. Script do Gateway IoT (Raspberry Pi 4)Este script Python roda no Raspberry Pi, interpretando as strings seriais recebidas pelo cabo USB e injetando a complexidade do contexto NGSI-LD (ETSI) antes de enviar ao servidor.Requisitos no Raspberry Pi:Bashsudo apt update && sudo apt install python3-pip -y
pip3 install pyserial requests
Arquivo: gateway_orion.py
Python
import serial
import json
import requests
import time

# ================= CONFIGURAÇÕES =================
PORTA_USB = '/dev/ttyUSB0' # Pode ser /dev/ttyUSB1 dependendo de onde plugar
BAUD_RATE = 115200
ORION_URL = 'http://<ip aqui>/ngsi-ld/v1/entityOperations/upsert?options=update'
# =================================================

def enviar_para_fiware(presence, distance):
    headers = {
        'Content-Type': 'application/ld+json',
        'NGSILD-Tenant': 'openiot'
    }
    
    # Montagem estrita do padrão NGSI-LD
    payload = [{
        "id": "urn:ngsi-ld:Device:LD2420",
        "type": "Device",
        "presence": {
            "type": "Property",
            "value": presence
        },
        "distance": {
            "type": "Property",
            "value": distance,
            "unitCode": "CMT"
        },
        "@context": [
            "https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld"
        ]
    }]

    try:
        response = requests.post(ORION_URL, json=payload, headers=headers, timeout=5)
        if response.status_code in [200, 201, 204]:
            print(f"[FIWARE OK] Presença: {presence} | Distância: {distance} cm")
        else:
            print(f"[FIWARE ERRO HTTP {response.status_code}] {response.text}")
    except requests.exceptions.RequestException as e:
        print(f"[ERRO DE REDE] Servidor inalcançável: {e}")

def main():
    print(f"Iniciando FIWARE Gateway na porta {PORTA_USB}...")
    
    try:
        ser = serial.Serial(PORTA_USB, BAUD_RATE, timeout=1)
        time.sleep(2) # Pausa de estabilização do boot do ESP8266
        print("Serial conectada. Processando telemetria...")
    except Exception as e:
        print(f"Erro ao abrir porta USB: {e}")
        return

    while True:
        try:
            if ser.in_waiting > 0:
                linha = ser.readline().decode('utf-8').strip()
                if not linha:
                    continue

                try:
                    dados = json.loads(linha)
                    # Verifica se o JSON tem as chaves mapeadas pelo ESP8266
                    if "presence" in dados and "distance" in dados:
                        enviar_para_fiware(dados["presence"], dados["distance"])
                    else:
                        print(f"[LOG ESP8266]: {linha}")
                        
                except json.JSONDecodeError:
                    print(f"[SERIAL RAW]: {linha}")
                    
        except serial.SerialException:
            print("[ERRO HARDWARE] Cabo USB desconectado do Raspberry Pi!")
            break
        except KeyboardInterrupt:
            print("\nGateway finalizado pelo usuário.")
            break

if __name__ == "__main__":
    main()
Para executar:Bashpython3 gateway_orion.py
