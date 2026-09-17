Projeto LD2420 + ESP8266 + FIWARE Orion

Documentação técnica completa da integração de borda entre o sensor radar mmWave Hi-Link LD2420, microcontrolador ESP8266 e o broker de contexto FIWARE Orion via protocolo NGSIv2.
1. Arquitetura da Solução

[ Sensor LD2420 ] 
       │ UART (38400 baud)
       ▼
[ ESP8266 NodeMCU ] ─── WiFi (HTTP POST NGSIv2) ───► [ FIWARE Orion ]
  - Deadband (>= 5cm)                                  (192.168.0.95:31330)
  - Throttling (500ms)                                       │
  - Edge Trigger                                             ▼
                                                     [ Túnel Pinggy ]
                                                             │
                                                             ▼
                                                    [ Celular / 4G-5G ]

    Camada de Sensoriamento: Leitura de micro-ondas a 24 GHz via UART para extração de distância e detecção de presença humana.

    Processamento de Borda: Tratamento de ruído e contenção de tráfego de rede no firmware do ESP8266 antes de acionar a pilha TCP/IP.

    Camada de Contexto: Persistência do estado do dispositivo em tempo real na entidade NGSIv2 dentro do Orion Context Broker.

    Exposição Remota: Túnel reverso SSH via Pinggy originado na estação de trabalho para inspeção e testes remotos via dispositivo móvel.

2. Pinagem e Hardware
Dispositivo	Pino LD2420	Pino ESP8266	Função no Firmware
Alimentação	VCC	VIN / VU (5V) ou 3V3	Alimentação do Radar
Alimentação	GND	GND	Referência Comum
Comunicação	TX	GPIO 13 (D7)	SoftwareSerial RX
Comunicação	RX	GPIO 15 (D8)	SoftwareSerial TX
3. Método Lógico do Algoritmo

O firmware não faz polling contínuo de envio de rede. A transmissão é condicionada por três regras combinadas:

    Filtro de Zona Morta (Deadband):
    Δd=∣datual​−danterior​∣≥5 cm

    Flutuações inferiores a 5 cm são descartadas na memória volátil, eliminando falsos positivos decorrentes de micro-movimentos respiratórios ou ruído térmico.

    Detecção de Borda (Edge Triggering):

        Borda de Subida: Primeira detecção registrada (lastDistance == -1).

        Borda de Variação: Alvo se movimentou além da tolerância estabelecida (≥5 cm).

        Borda de Descida: Alvo saiu do raio de leitura. Dispara evento único com presence: false e distance: 0, resetando os registradores.

    Throttling Não-Bloqueante:
    Uso do contador de sistema millis() com taxa de amostragem mínima de 500 ms entre requisições HTTP sucessivas, mantendo o barramento serial responsivo a cada 10 ms.

4. Código-Fonte do Microcontrolador (ESP8266)
C++

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>
#include "LD2420.h"

// Definição de pinos
#define RX_PIN 13 // Conectado ao TX do LD2420
#define TX_PIN 15 // Conectado ao RX do LD2420

SoftwareSerial sensorSerial(RX_PIN, TX_PIN);
LD2420 radar;

// Configurações de Rede e Broker
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";
const char* orionUrl = "http://192.168.0.95:31330/v2/op/update";

// Controle de Estado
int lastDistance = -1;
bool wasDetecting = false;

// Controle de Vazão e Filtro
unsigned long ultimoEnvio = 0;
const int tempoMinimoEntreEnvios = 500; 
const int LIMITE_MUDANCA_DISTANCIA = 5;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println("\n=== LD2420 + FIWARE Orion ===");

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[Wi-Fi] Conectado!");

  sensorSerial.begin(38400);
  if (radar.begin(sensorSerial)) {
    Serial.println("[LD2420] Radar pronto.");
  } else {
    Serial.println("[LD2420] Falha na inicializacao!");
    while (1) delay(1000);
  }

  radar.setDistanceRange(0, 600);
  radar.setUpdateInterval(50);
}

void enviarParaOrion(bool isDetecting, int currentDistance) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;

  http.begin(client, orionUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("fiware-service", "openiot");
  http.addHeader("fiware-servicepath", "/");

  // Payload NGSIv2 (Upsert via APPEND)
  String payload = "{\"actionType\":\"APPEND\",\"entities\":[{\"id\":\"urn:ngsi-ld:Device:LD2420\",\"type\":\"Device\",";
  payload += "\"presence\":{\"type\":\"Boolean\",\"value\":" + String(isDetecting ? "true" : "false") + "},";
  payload += "\"distance\":{\"type\":\"Number\",\"value\":" + String(currentDistance) + "}}]}";

  int httpCode = http.POST(payload);

  if (httpCode == 204) {
    Serial.printf("[FIWARE OK] Presenca: %s | Distancia: %d cm\n", isDetecting ? "Sim" : "Nao", currentDistance);
  } else {
    Serial.printf("[FIWARE ERRO] HTTP Code: %d\n", httpCode);
  }

  http.end();
}

void loop() {
  radar.update();

  bool isDetecting = radar.isDetecting();
  int currentDistance = radar.getDistance();

  if (millis() - ultimoEnvio > tempoMinimoEntreEnvios) {
    if (isDetecting) {
      if (lastDistance == -1 || abs(currentDistance - lastDistance) >= LIMITE_MUDANCA_DISTANCIA) {
        Serial.printf("Movimento significativo: %d cm (Anterior: %d cm)\n", currentDistance, lastDistance);
        enviarParaOrion(true, currentDistance);
        lastDistance = currentDistance;
        wasDetecting = true;
        ultimoEnvio = millis();
      }
    } else {
      if (wasDetecting) {
        Serial.println("Movimento finalizado. Resetando estado.");
        enviarParaOrion(false, 0);
        lastDistance = -1;
        wasDetecting = false;
        ultimoEnvio = millis();
      }
    }
  }

  delay(10);
}

5. Monitoramento Local e Validação

Executar na estação de trabalho (nerds-workstation) conectada à mesma sub-rede:
Consulta Pontual (cURL)
Bash

curl -X GET "http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:Device:LD2420" \
  -H "fiware-service: openiot" \
  -H "fiware-servicepath: /"

Dashboard Dinâmico via Terminal

Atualiza os valores a cada 2 segundos destacando mudanças em tempo real:
Bash

watch -n 2 -d 'curl -s -X GET "http://192.168.0.95:31330/v2/entities/urn:ngsi-ld:Device:LD2420" -H "fiware-service: openiot" -H "fiware-servicepath: /"'

6. Acesso Externo Móvel (Túnel SSH Reverso)

Permite consultar os dados do broker em redes móveis (4G/5G) sem necessidade de portas abertas no roteador.
Inicialização do Túnel na Estação Local
Bash

ssh -p 443 -R0:192.168.0.95:31330 a.pinggy.io

Requisição Mobile (via API Tester / HTTP Shortcuts)

    Método: GET

    URL: https://<URL_GERADA_PELO_PINGGY>/v2/entities/urn:ngsi-ld:Device:LD2420

    Headers:

        fiware-service: openiot

        fiware-servicepath: /

Exemplo de Resposta NGSIv2 Válida
JSON

{
  "id": "urn:ngsi-ld:Device:LD2420",
  "type": "Device",
  "distance": {
    "type": "Number",
    "value": 142,
    "metadata": {}
  },
  "presence": {
    "type": "Boolean",
    "value": true,
    "metadata": {}
  }
}

7. Próximos Passos e Roadmap

    Séries Temporais (Persistência Histórica):

        Configuração de instâncias QuantumLeap + CrateDB ou TimescaleDB.

        Criação de Subscription NGSIv2 para persistir curvas de distância ao longo do tempo.

    Camada de Visualização:

        Montagem de painel gráfico no Grafana consumindo as métricas do banco temporal.

        Alternativa leve: Dashboard em página estática (HTML/JavaScript) consultando o endpoint via polling.

    Mecanismo de Ações e Alertas:

        Configuração de Subscriptions com gatilho condicional para acionamento de atuadores ou envio de webhooks externos quando d≤X cm.

    Hardening do Firmware:

        Inclusão de WiFi.setAutoReconnect(true) e rotina de non-blocking recovery para quedas de sinal.

        Implementação de Watchdog de Hardware (WDT) para prevenção de travamentos na camada TCP.
