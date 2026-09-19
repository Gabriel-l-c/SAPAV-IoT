Veja o [NGSI-LD Guidelines for the deployment of Smart City and Communities data platforms](gr_CIM020v010101p.pdf).


Veja o [NGSI-LD API](gs_cim009v010601p.pdf).

Acesse o [cyrixninja](https://github.com/cyrixninja/LD2420/tree/main) para a biblioteca do ld2420.

Acesso o [biblioteca esp8266wifi](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html) para o documento de descrição da biblioteca.

Acesso o [ NGSI-LD for NGSI-v2 Developers » Linked Data](https://fiware-tutorials.readthedocs.io/en/latest/linked-data.html) para descricao do NGSI-LD no site do fiware.

**identificar a versao do firware no ip **
curl -s -X GET "http://<ip aqui>/version"
{
  "orionld version": "1.8.0",
  "orion version":   "1.15.0-next",
  "uptime":          "1 d, 0 h, 15 m, 14 s",
  "git_hash":        "nogitversion",
  "compile_time":    "Mon Jan 13 11:28:16 UTC 2025",
  "compiled_by":     "root",
  "compiled_in":     "",
  "release_date":    "Mon Jan 13 11:28:16 UTC 2025",
  "doc":             "https://fiware-orion.readthedocs.org/en/master/"
}


**limpar o ip**
curl -i -X DELETE "http://<ip aqui>/ngsi-ld/v1/entities/urn:ngsi-ld:Device:LD2420" \
  -H "NGSILD-Tenant: openiot"


**upgrades do dia **

As principais alterações incluem:

    Endpoint: Alterado para /ngsi-ld/v1/entityOperations/upsert?options=update.

    Header: Content-Type: application/ld+json.

    Tenant (opcional): Inclusão do header NGSILD-Tenant: openiot para suporte a multi-tenancy.

    Payload: Atributos estruturados como Property, unidade CMT (centímetros segundo UN/CEFACT) e injeção do @context core oficial.

C++

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>
#include "LD2420.h"

// Pinos originais
#define RX_PIN 13 // Conectado ao TX do LD2420
#define TX_PIN 15 // Conectado ao RX do LD2420

SoftwareSerial sensorSerial(RX_PIN, TX_PIN);
LD2420 radar;

// Configurações de Rede e Orion-LD
const char* ssid = "SUA_REDE_WIFI";
const char* password = "SUA_SENHA_WIFI";

// Endpoint de Upsert em lote do NGSI-LD
const char* orionLdUrl = "http://<ip aqui>/ngsi-ld/v1/entityOperations/upsert?options=update";

// Variáveis de Controle de Estado
int lastDistance = -1;
bool wasDetecting = false;

// Controle de Vazão e Filtro Deadband
unsigned long ultimoEnvio = 0;
const int tempoMinimoEntreEnvios = 500; 
const int LIMITE_MUDANCA_DISTANCIA = 5; // Tolerância de 5 cm

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  Serial.println("\n=== LD2420 (ESP8266) -> Orion-LD (NGSI-LD) ===");

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
    Serial.println("[LD2420] Falha ao inicializar!");
    while (1) delay(1000);
  }

  radar.setDistanceRange(0, 600);
  radar.setUpdateInterval(50);
}

void enviarParaOrionLD(bool isDetecting, int currentDistance) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClient client;
  HTTPClient http;

  http.begin(client, orionLdUrl);
  
  // Headers oficiais NGSI-LD
  http.addHeader("Content-Type", "application/ld+json");
  http.addHeader("NGSILD-Tenant", "openiot");

  // Montagem do Payload NGSI-LD dentro de um Array (Batch Operation)
  String payload = "[{";
  payload += "\"id\":\"urn:ngsi-ld:Device:LD2420\",";
  payload += "\"type\":\"Device\",";
  payload += "\"presence\":{";
  payload +=   "\"type\":\"Property\",";
  payload +=   "\"value\":" + String(isDetecting ? "true" : "false");
  payload += "},";
  payload += "\"distance\":{";
  payload +=   "\"type\":\"Property\",";
  payload +=   "\"value\":" + String(currentDistance) + ",";
  payload +=   "\"unitCode\":\"CMT\"";
  payload += "},";
  payload += "\"@context\":[";
  payload +=   "\"https://uri.etsi.org/ngsi-ld/v1/ngsi-ld-core-context.jsonld\"";
  payload += "]";
  payload += "}]";

  int httpCode = http.POST(payload);

  // No NGSI-LD upsert: 200 (Atualizado), 201 (Criado) ou 204 (Sem conteúdo) indicam sucesso
  if (httpCode == 200 || httpCode == 201 || httpCode == 204) {
    Serial.printf("[NGSI-LD OK - %d] Presenca: %s | Distancia: %d cm\n", 
                  httpCode, isDetecting ? "true" : "false", currentDistance);
  } else {
    Serial.printf("[NGSI-LD ERRO] Código HTTP: %d | Resposta: %s\n", 
                  httpCode, http.getString().c_str());
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
        Serial.printf("[Evento] Detectado: %d cm (Anterior: %d cm)\n", currentDistance, lastDistance);
        enviarParaOrionLD(true, currentDistance);
        lastDistance = currentDistance;
        wasDetecting = true;
        ultimoEnvio = millis();
      }
    } else {
      if (wasDetecting) {
        Serial.println("[Evento] Movimento cessado. Resetando estado.");
        enviarParaOrionLD(false, 0);
        lastDistance = -1;
        wasDetecting = false;
        ultimoEnvio = millis();
      }
    }
  }

  delay(10);
}



**acesso a os dados enviados para o broker**

curl -X GET "http://<ip aqui>/ngsi-ld/v1/entities/urn:ngsi-ld:Device:LD2420" \
  -H "NGSILD-Tenant: openiot" \
  -H "Accept: application/ld+json"


**assistir atualização do sensor em tempo real**

watch -n 2 -d 'curl -s -X GET "http://<ip aqui>/v2/entities/urn:ngsi-ld:Device:LD2420" -H "fiware-service: openiot" -H "fiware-servicepath: /"'
