#include <HardwareSerial.h>

#define RADAR_RX_PIN 16  // RX pin o
#define RADAR_TX_PIN 17  // TX pin 
#define OT2_PIN 27
#define BUFFER_SIZE 256  // buffer size
#define LED_PIN 25

char incomingBuffer[BUFFER_SIZE];  // Buffer to store incoming data
int bufferIndex = 0;               // Index to keep track of buffer position
bool startReading = true;

void setup() {
  pinMode(OT2_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);                                          
  Serial2.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);  
  delay(200);                                           
  Serial.println("Starting...");
  Serial2.println("test");
}

void loop() {  // Check if data is available on Serial2
  startReading = true;
  if (Serial2.available()) {
    while (Serial2.available() && startReading) {
      char incomingByte = Serial2.read();  // Read a byte
      if (incomingByte == '\n') {          // If newline character is detected, terminate the string and process the message
        incomingBuffer[bufferIndex] = '\0';
        onReceiveFinished(String(incomingBuffer));
        bufferIndex = 0;  // Reset buffer index for the next message
        startReading = false;
      } else {  // Add the byte to the buffer if there's space
        if (bufferIndex < BUFFER_SIZE - 1) {
          incomingBuffer[bufferIndex++] = incomingByte;
        }
      }
    }
  }
}

void onReceiveFinished(String message) {
  Serial.print("Received message: ");
  Serial.println(message);
}
