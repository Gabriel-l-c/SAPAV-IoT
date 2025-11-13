/*
 * BasicUsage.ino - Basic example for LD2420 Radar Sensor Library - ESP32 Version
 * 
 * This example demonstrates the basic usage of the LD2420 library on ESP32.
 * It shows how to initialize the sensor, read distance data, and use callbacks.
 * 
 * Hardware connections for ESP32:
 * - LD2420 VCC -> 3.3V
 * - LD2420 GND -> GND  
 * - LD2420 TX  -> Pin 16 (RX2)
 * - LD2420 RX  -> Pin 17 (TX2)
 * 
 * Author: cyrixninja
 * Date: July 22, 2025
 * Adapted for ESP32: [Mauro Miyashiro/DeepSeek]
 */

#include <HardwareSerial.h>
#include "LD2420.h"

// For ESP32, we can use HardwareSerial instead of SoftwareSerial
#define RX_PIN 16
#define TX_PIN 17

// Create HardwareSerial instance (UART2 on ESP32)
HardwareSerial sensorSerial(2);
LD2420 radar;

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for Serial Monitor
  }
  
  Serial.println("=== LD2420 Radar Sensor Example - ESP32 ===");
  
  // Initialize HardwareSerial for sensor communication
  sensorSerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // Initialize the radar sensor
  if (radar.begin(sensorSerial)) {
    Serial.println("✓ LD2420 initialized successfully!");
    Serial.println(radar.getVersionInfo());
  } else {
    Serial.println("✗ Failed to initialize LD2420!");
    while (1) delay(1000); // Stop execution
  }
  
  // Configure distance range (optional)
  radar.setDistanceRange(0, 400); // 0-400 cm range
  
  // Set update interval (optional)
  radar.setUpdateInterval(50); // Update every 50ms
  
  // Set up callbacks (commented out if not available in your LD2420 library version)
  // radar.onDetection(onObjectDetected);
  // radar.onStateChange(onStateChanged);
  // radar.onDataUpdate(onDataReceived);
  
  Serial.println("Setup complete. Waiting for detections...");
  Serial.println("----------------------------------------");
}

void loop() {
  // Update the radar sensor (call this regularly)
  radar.update();
  
  // You can also read data directly
  if (radar.isDataValid()) {
    LD2420_Data data = radar.getCurrentData();
    
    // Print status every 2 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
      printStatus(data);
      lastPrint = millis();
    }
  }
  
  delay(10);
}

// Callback function called when an object is detected
void onObjectDetected(int distance) {
  Serial.print("🎯 Object detected at ");
  Serial.print(distance);
  Serial.println(" cm");
}

// Callback function called when detection state changes
void onStateChanged(LD2420_DetectionState oldState, LD2420_DetectionState newState) {
  Serial.print("📡 State change: ");
  Serial.print(stateToString(oldState));
  Serial.print(" -> ");
  Serial.println(stateToString(newState));
}

// Callback function called when new data is received
void onDataReceived(LD2420_Data data) {
  // This callback is called for every data update
  // You can use this for logging or processing all data
}

// Helper function to print current status
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
