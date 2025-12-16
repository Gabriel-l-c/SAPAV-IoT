// -----------------------------------------------------------
// 1. INCLUSÕES E DEFINIÇÕES CONDICIONAIS
// -----------------------------------------------------------

#include "LD2420.h"
#include <PubSubClient.h>
// Add the necessary WiFi includes here based on the target board
#if defined(ESP32)
  #include <WiFi.h>          // <-- MOVED HERE (for ESP32)
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>   // <-- MOVED HERE (for ESP8266)
#endif
//led para testes caso necessario
// #define LedPin      3                 // ESP32 in-board led pin

const char* ssid = "nerds";                     // Change this to your Wifi SSID
const char* password = "nerds23517891";            // Change this to your Wifi Password
const char* mqtt_server = "test.mosquitto.org"; // Mosquitto Server URL
const char* mqtt_topic_publish = "esp32/sensor/distance";  
// float sensorValue; // <-- REMOVED: We will use data.distance directly or define it as an int if needed.
WiFiClient espClient;
PubSubClient client(espClient);

#if defined(ESP32)
  #include <HardwareSerial.h>
  // <WiFi.h> is no longer needed here
  #define RX_PIN 16 // Conecta ao TX do LD2420
  #define TX_PIN 17 // Conecta ao RX do LD2420
  HardwareSerial sensorSerial(2);
#elif defined(ESP8266)
  #include <SoftwareSerial.h>
  // <ESP8266WiFi.h> is no longer needed here
  #define RX_PIN 13 // Conecta ao TX do LD2420
  #define TX_PIN 15 // Conecta ao RX do LD2420
  SoftwareSerial sensorSerial(RX_PIN, TX_PIN);
#else
  #error "Esta placa não é um ESP32 nem um ESP8266!"
#endif

// Instância da biblioteca LD2420
LD2420 radar;

// ... (Rest of the auxiliary functions: setup_wifi, reconnect, onObjectDetected, etc. - kept the same)
void setup_wifi()
{ 
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.print(ssid);
    WiFi.begin(ssid, password);

    while(WiFi.status() != WL_CONNECTED) 
    { 
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() 
{ 
  while(!client.connected()) 
  {
      Serial.println("Attempting MQTT connection...");

      if(client.connect("ESPClient")) 
      {
          Serial.println("Connected");
          client.subscribe("/LedControl");
      } 
      else 
      {
          Serial.print("Failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
          delay(5000);
      }
    }
}

void setup() {
  // Inicializa o Serial Monitor
  Serial.begin(115200);
  while (!Serial) {
    delay(10); 
  }
  
  // Imprime a placa que está sendo usada
  #if defined(ESP32)
    Serial.println("=== LD2420 Radar Sensor Example - ESP32 ===");
    // Inicializa HardwareSerial para sensor (UART2 com remapeamento de pinos)
    sensorSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  #elif defined(ESP8266)
    Serial.println("=== LD2420 Radar Sensor Example - ESP8266 (SoftwareSerial) ===");
    // Inicializa SoftwareSerial para sensor (apenas baud rate)
    sensorSerial.begin(115200);
  #endif
  
  // Inicializa o sensor radar
  if (radar.begin(sensorSerial)) {
    Serial.println("✓ LD2420 initialized successfully!");
    // Exemplo de como chamar o getVersionInfo()
    Serial.println(radar.getVersionInfo()); 
  } else {
    Serial.println("✗ Failed to initialize LD2420!");
    while (1) delay(1000); // Para a execução
  }
  
  // Configurações do Radar
  radar.setDistanceRange(0, 800); // 0-800 cm range (updated from your comment)
  radar.setUpdateInterval(50);     // Update every 50ms
  
  // Configura callbacks (descomente se estiver disponível na sua lib)
  // radar.onDetection(onObjectDetected);
  // radar.onStateChange(onStateChanged);
  // radar.onDataUpdate(onDataReceived);
  
  Serial.println("Setup complete. Waiting for detections...");
  Serial.println("----------------------------------------");

  // pinMode(LedPin, OUTPUT);
  setup_wifi(); 
  client.setServer(mqtt_server, 1883);
  // client.setCallback(callback);
  // digitalWrite(LedPin, LOW);
}

// -----------------------------------------------------------
// 3. LOOP PRINCIPAL COM PUBLICAÇÃO MQTT CORRIGIDA
// -----------------------------------------------------------
void loop() {
  // Atualiza o sensor radar (chamada obrigatória regularmente)
  radar.update();
  
  // Leitura direta dos dados
  if (radar.isDataValid()) {
    LD2420_Data data = radar.getCurrentData();
    
    // Imprime o status a cada 2 segundos
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
      printStatus(data);
      lastPrint = millis();
      
      // 1. Get the actual integer distance value (in cm)
      int distanceCm = data.distance; 

      // 2. Convert integer to C-string using itoa()
      char distString[8]; // Buffer for the string
      itoa(distanceCm, distString, 10); // Base 10 (decimal) conversion

      // 3. Publish the data
      Serial.print("Publishing distance: ");
      Serial.print(distanceCm); 
      Serial.print(" cm -> MQTT Topic: ");
      Serial.println(mqtt_topic_publish);
      
      client.publish(mqtt_topic_publish, distString);
    }
  }
  
  delay(10);
  if(!client.connected()) { reconnect(); }
    client.loop();
}

// ... (Keep the rest of your auxiliary functions: onObjectDetected, onStateChanged, printStatus, stateToString)
void onObjectDetected(int distance) {
  Serial.print("🎯 Object detected at ");
  Serial.print(distance);
  Serial.println(" cm");
}

void onStateChanged(LD2420_DetectionState oldState, LD2420_DetectionState newState) {
  Serial.print("📡 State change: ");
  Serial.print(stateToString(oldState));
  Serial.print(" -> ");
  Serial.println(stateToString(newState));
}

void onDataReceived(LD2420_Data data) {
    // This callback is called for every data update
  // You can use this for logging or processing all data
}

void printStatus(LD2420_Data data) {
  Serial.println("--- Current Status ---");
  Serial.print("Distance: ");
  Serial.print(data.distance);
  Serial.println(" cm");
  
  Serial.print("State: ");
  Serial.println(stateToString(data.state));
  
  Serial.print("Last update: ");
  Serial.print(millis() - data.timestamp);
  Serial.println(" ms ago");
  
  Serial.print("Data valid: ");
  Serial.println(data.isValid ? "Yes" : "No");
  Serial.println("---------------------");
}

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