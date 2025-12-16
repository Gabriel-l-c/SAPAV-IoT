/*
 * BasicUsage.ino - Basic example for LD2420 Radar Sensor Library - ESP32 Version
 * * Este código foi modificado para incluir conexão Wi-Fi e MQTT para Node-RED.
 * Os dados de distância e estado de detecção do radar são publicados em tópicos MQTT.
 * * Hardware connections for ESP32:
 * - LD2420 VCC -> 3.3V
 * - LD2420 GND -> GND  
 * - LD2420 TX  -> Pin 16 (RX2)
 * - LD2420 RX  -> Pin 17 (TX2)
 * * Autor: cyrixninja
 * Date: July 22, 2025
 * Adapted for ESP32: [Mauro Miyashiro/DeepSeek]
 * Adicionado MQTT por: Gemini
 */

#include <HardwareSerial.h>
#include "LD2420.h"

// **********************************************
// ********* Configurações de Wi-Fi e MQTT ********
// **********************************************
#include <WiFi.h>
#include <PubSubClient.h>

// Configurações da sua Rede Wi-Fi
const char* ssid = "nerds";          // <<<<<<< MUDAR
const char* password = "nerds23517891";     // <<<<<<< MUDAR

// Configurações do Broker MQTT
const char* mqtt_server = "172.16.30.83"; // Ex: "192.168.1.100" ou um nome de host
const int mqtt_port = 1883;                     // Porta padrão do MQTT
const char* mqtt_client_id = "LD2420_Radar_ESP32"; // ID único para o cliente

// Tópicos MQTT para envio de dados
const char* mqtt_topic_distance = "radar/ld2420/distance_cm";
const char* mqtt_topic_state = "radar/ld2420/state";

const char* mqtt_user = "nerd";     // <<<<<<< MUDAR
const char* mqtt_pass = "nerd135798642";  // <<<<<<< MUDAR
// Objetos para Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);
// **********************************************

// Para ESP32, podemos usar HardwareSerial
#define RX_PIN 16
#define TX_PIN 17

// Criar instância HardwareSerial (UART2 no ESP32)
HardwareSerial sensorSerial(2);
LD2420 radar;

// Variáveis de controle para o MQTT
long lastMsg = 0;
char msgBuffer[50]; // Buffer para mensagens MQTT

// Prototipos de funções para organização
void setup_wifi();
void reconnect();
void publishData(LD2420_Data data);
String stateToString(LD2420_DetectionState state);
void printStatus(LD2420_Data data);

void setup() {
  // Inicializar Serial Monitor
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Aguardar Serial Monitor
  }
  
  Serial.println("=== LD2420 Radar Sensor Example - ESP32 com MQTT ===");
  
  // 1. Configurar Wi-Fi
  setup_wifi();

  // 2. Configurar MQTT
  client.setServer(mqtt_server, mqtt_port);
  // client.setCallback(callback); // Não usaremos callback, apenas publicação

  // 3. Inicializar HardwareSerial para comunicação com o sensor
  sensorSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // 4. Inicializar o sensor radar
  if (radar.begin(sensorSerial)) {
    Serial.println("✓ LD2420 inicializado com sucesso!");
    Serial.println(radar.getVersionInfo());
  } else {
    Serial.println("✗ Falha ao inicializar LD2420!");
    while (1) delay(1000); // Parar execução
  }
  
  // Configurar faixa de distância (opcional)
  radar.setDistanceRange(0, 400); // 0-400 cm
  
  // Definir intervalo de atualização (opcional)
  radar.setUpdateInterval(50); // Atualizar a cada 50ms
  
  Serial.println("Setup completo. Aguardando detecções...");
  Serial.println("----------------------------------------");
}

void loop() {
  // Garantir a conexão MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // Manter a conexão MQTT ativa

  // Atualizar o sensor radar (chamar regularmente)
  radar.update();
  
  // Lógica de leitura e publicação de dados
  if (radar.isDataValid()) {
    LD2420_Data data = radar.getCurrentData();
    
    // Publicar e imprimir status a cada 2 segundos (ou outro intervalo)
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) { // Intervalo de publicação (2000 ms = 2s)
      publishData(data); // Publica os dados via MQTT
      printStatus(data); // Imprime os dados no Serial Monitor
      lastUpdate = millis();
    }
  }
  
  delay(10);
}

// **********************************************
// ********* Funções de Wi-Fi e MQTT ************
// **********************************************

void setup_wifi() {
  delay(10);
  // Conectar ao Wi-Fi
  Serial.print("Conectando-se a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Conexão Wi-Fi estabelecida.");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop até reconectar
  while (!client.connected()) {
    Serial.print("Tentando conexão MQTT...");
    // Tenta conectar
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_pass)) {
      Serial.println("conectado.");
    } else {
      Serial.print("falhou, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos.");
      // Espera 5 segundos antes de tentar novamente
      delay(5000);
    }
  }
}

void publishData(LD2420_Data data) {
  if (client.connected()) {
    // 1. Publicar Distância (como string)
    dtostrf(data.distance, 4, 0, msgBuffer); // Converte float (ou int, dependendo da lib) para string
    client.publish(mqtt_topic_distance, msgBuffer);
    
    // 2. Publicar Estado (como string)
    String stateStr = stateToString(data.state);
    stateStr.toCharArray(msgBuffer, 50);
    client.publish(mqtt_topic_state, msgBuffer);
  }
}

// **********************************************
// ********* Funções Auxiliares do Sensor *******
// **********************************************

// Helper function to print current status
void printStatus(LD2420_Data data) {
  Serial.println("--- Current Status ---");
  Serial.print("Distance: ");
  Serial.print(data.distance);
  Serial.print(" cm (MQTT Topic: ");
  Serial.print(mqtt_topic_distance);
  Serial.println(")");
  
  Serial.print("State: ");
  Serial.print(stateToString(data.state));
  Serial.print(" (MQTT Topic: ");
  Serial.print(mqtt_topic_state);
  Serial.println(")");
  
  Serial.print("Last update: ");
  Serial.print(millis() - data.timestamp);
  Serial.println(" ms ago");
  
  Serial.print("Data valid: ");
  Serial.println(data.isValid ? "Yes" : "No");
  Serial.println("---------------------");
}

// Helper function to convert state to string
String stateToString(LD2420_DetectionState state) {
  switch (state) {
    case LD2420_NO_DETECTION:
      return "No Detection";
    case LD2420_DETECTION_ACTIVE:
      return "Active Detection";
    case LD2420_DETECTION_LOST:
      return "Detection Lost";
    default:
      return "Unknown";
  }
}

// As funções de callback originais foram removidas do loop/setup para simplificar
// o foco na publicação MQTT, mas podem ser reintroduzidas se necessário.