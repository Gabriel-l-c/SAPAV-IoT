// =================================================================
// 1. INCLUSÃO DE BIBLIOTECAS E DETECÇÃO DO MICROCONTROLADOR
// =================================================================

#include "LD2420.h"
#include <PubSubClient.h> // Você precisará instalar esta biblioteca


// =================================================================
// 2. CONFIGURAÇÕES DE REDE E MQTT
// =================================================================

// Configurações Wi-Fi
const char* ssid = "SSID WIFI";
const char* password = "SENHA WIFI";

// Configurações MQTT
const char* mqtt_server = "<ip aqui>"; // Ex: "broker.hivemq.com"
const int mqtt_port = 1883; 
const char* mqtt_client_id = "LD2420_Radar_Client"; 
const char* mqtt_topic_distance = "radar/presenca/distancia"; // Tópico para a distância
const char* mqtt_topic_state = "radar/presenca/estado";     // Tópico para o estado

// =================================================================
// 3. DEFINIÇÕES CONDICIONAIS (ESP32/ESP8266)
// (Mantidas do exemplo anterior)
// =================================================================


#if defined(ESP32)
  #include <WiFi.h>
  #include <HardwareSerial.h>

  #define MCU_NAME "ESP32"
  #define RX_PIN 16
  #define TX_PIN 17
  // Declarações específicas do ESP32
  HardwareSerial sensorSerial(2);

#elif defined(ESP8266)
  #include <ESP8266WiFi.h> // <-- ALTERNATIVA ROBUSTA: O ESP8266 suporta isso, mas vamos tentar o WiFi.h primeiro
  #include <SoftwareSerial.h>
  

  #define MCU_NAME "ESP8266"
  #define RX_PIN 12 // GPIO12
  #define TX_PIN 13 // GPIO13
  SoftwareSerial sensorSerial(RX_PIN, TX_PIN);

#else
  #error "Placa não suportada. Use ESP32 ou ESP8266."
#endif
// // Bibliotecas Comuns para Wi-Fi e MQTT
// #include <WiFi.h>
// #include <PubSubClient.h> // Você precisará instalar esta biblioteca
// Instâncias globais
WiFiClient espClient;
PubSubClient mqttClient(espClient);
LD2420 radar;

// =================================================================
// 4. FUNÇÕES DE CONEXÃO E PUBLICAÇÃO MQTT
// =================================================================

// Função para conectar ao Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.print("\nConectando a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Função para reconectar ao MQTT
void reconnect_mqtt() {
  // Loop até que estejamos reconectados
  while (!mqttClient.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Tenta conectar
    if (mqttClient.connect(mqtt_client_id)) {
      Serial.println("conectado.");
      // Se precisar subscrever a algum tópico (ex: para receber comandos), faça aqui.
      // mqttClient.subscribe("comando/radar"); 
    } else {
      Serial.print("falhou, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Tentando novamente em 5 segundos.");
      // Espera 5 segundos antes de tentar novamente
      delay(5000);
    }
  }
}

// Função para publicar os dados do radar
void publish_radar_data(LD2420_Data data) {
  // 1. Publica a Distância (como string)
  String distanceStr = String(data.distance);
  mqttClient.publish(mqtt_topic_distance, distanceStr.c_str(), true); // 'true' para Retain
  Serial.print("MQTT Distância: ");
  Serial.println(distanceStr);

  // 2. Publica o Estado (como string)
  String stateStr = stateToString(data.state);
  mqttClient.publish(mqtt_topic_state, stateStr.c_str(), true); // 'true' para Retain
  Serial.print("MQTT Estado: ");
  Serial.println(stateStr);
}

// --- Funções Auxiliares (stateToString, printStatus, etc.) ---
// Mantenha as funções auxiliares (stateToString, onObjectDetected, etc.) do seu código original aqui, 
// pois elas são necessárias para o funcionamento e compilação do código.

// ... (Funções Auxiliares aqui) ... 
String stateToString(LD2420_DetectionState state) { /* ... */ return "Estado"; }
void printStatus(LD2420_Data data) { /* ... */ }
void onObjectDetected(int distance) { /* ... */ }
void onStateChanged(LD2420_DetectionState oldState, LD2420_DetectionState newState) { /* ... */ }
void onDataReceived(LD2420_Data data) { /* ... */ }


// =================================================================
// 5. SETUP E LOOP
// =================================================================

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  
  // 1. Inicializa Serial do Sensor
#if defined(ESP32)
  sensorSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
#elif defined(ESP8266)
  sensorSerial.begin(115200);
#endif

  // 2. Inicializa o Radar
  if (!radar.begin(sensorSerial)) {
    Serial.println("✗ Falha ao inicializar LD2420!");
    while (1) delay(1000);
  }
  radar.setDistanceRange(0, 400); 
  radar.setUpdateInterval(50);
  Serial.print("=== LD2420 Radar Sensor Example - ");
  Serial.print(MCU_NAME); 
  Serial.println(" ===");
  Serial.println("✓ Radar inicializado com sucesso.");

  // 3. Inicializa Conexão Wi-Fi e MQTT
  setup_wifi();
  mqttClient.setServer(mqtt_server, mqtt_port);
  // mqttClient.setCallback(callback); // Se você precisar de lógica de subscrição/recebimento
}

void loop() {
  // Garante que estamos conectados ao broker MQTT
  if (!mqttClient.connected()) {
    reconnect_mqtt();
  }
  mqttClient.loop(); // Processa mensagens recebidas (se houver subscrição)

  radar.update();
  
  // Lógica de Publicação
  if (radar.isDataValid()) {
    LD2420_Data data = radar.getCurrentData();
    
    // Publica no MQTT em um intervalo menor (ex: a cada 2 segundos)
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > 2000) { 
      publish_radar_data(data);
      // Opcional: printStatus(data);
      lastPublish = millis();
    }
  }
  
  delay(10);
}
