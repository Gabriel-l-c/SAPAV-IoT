#include <SPI.h>
#include <ESP8266WiFi.h>  // Use ESP8266-specific WiFi library
#include <WiFiClient.h>   // Required for making HTTP requests

/**************************/
/* Setup Configuration    */
/**************************/

// ⚠️ Replace these placeholders with your actual WiFi and Orion information
char ssid[] = "nerds";                   // Name of the network
char pass[] = "nerds23517891";           // Password of the network
char server[] = "172.16.30.87";                   // ORION IP Address (e.g., "130.206.80.47")

int status = WL_IDLE_STATUS;             // Predefine the connection status
// Using a common ESP8266 Digital Pin (D4 corresponds to GPIO2, often has an onboard LED)
// Pin 13 is safe, but D4 is a common default for the onboard LED on NodeMCU/Wemos boards.
int led = 13; 

// Function Declarations
void printWifiStatus();
String getLEDfromOrion();


/**
 * Arduino Setup configuration
 * (Execute only once)
 **/
void setup() {
  // Initialization of the Arduino serial port
  Serial.begin(9600);
  
  // Wait for serial port to connect (useful for debugging)
  while (!Serial) {
    ;
  }
  
  // Initialize the digital pin as an output
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW); // Start with LED off

  // --- WiFi Connection ---
  
  // Try to connect to the WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Trying to connect to SSID: ");
    Serial.println(ssid);
    
    // Connect to the network
    status = WiFi.begin(ssid, pass);
    delay(10000); // Wait 10 seconds to allow connection
  }
  
  // Once connected
  Serial.println("Connected to WiFi");
  printWifiStatus();
}


/**
 * Main code loop
 * (Execute repeatedly)
 **/
void loop() {
  // Update led state every 2 seconds
  delay(2000);

  // Check if WiFi dropped and reconnect if necessary
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    status = WL_IDLE_STATUS; // Reset status to force reconnection attempt
    while (status != WL_CONNECTED) {
      status = WiFi.begin(ssid, pass);
      delay(5000);
    }
    Serial.println("Reconnected to WiFi");
    printWifiStatus();
  }
  
  String buf;
  // Execute the call to the server
  buf = getLEDfromOrion();
  
  // --- Data Parsing ---
  // The goal is to find if the JSON response contains the state "true"
  buf.replace("\n","");
  buf.replace("\t","");
  
  // Find the start of the JSON body after the HTTP headers
  int startIndex = buf.indexOf('{');
  if (startIndex == -1) {
    Serial.println("Error: Could not find JSON body in response.");
    digitalWrite(led, LOW);
    return; // Exit loop iteration if parsing fails
  }
  
  String json = buf.substring(startIndex);
  Serial.println("JSON Response: " + json);
  
  // Check for the value. Assuming the response contains: {"id":"LED001", ..., "on":{"value":"true", ...}}
  // We search for a reliable fragment indicating the LED is ON.
  int val = json.indexOf("\"value\":\"true\""); 
  
  Serial.print("Found 'value:\"true\"' at index: ");
  Serial.println(val);
  
  if (val != -1) {
    // Orion responds that the LED is ON
    digitalWrite(led, HIGH);
    Serial.println("LED ON");
  } else {
    // Orion responds OFF, or the entity/value was not found
    digitalWrite(led, LOW);
    Serial.println("LED OFF");
  }
}


/**
 * Performs an HTTP GET request to the Orion Context Broker to fetch the state of LED001.
 * @return The raw HTTP response string including headers and body.
 **/
String getLEDfromOrion() {
  WiFiClient client;
  Serial.println("\nStarting connection with the Server...");
  
  // If I connect with the server on port 1026
  if (client.connect(server, 1026)) {
    Serial.println("Connected to the Server");
    
    // --- HTTP Request (NGSI-v2 Entity Retrieval) ---
    // Using NGSI-v2 endpoint is simpler for ESP8266 parsing
    client.println("GET /v2/entities/LED001 HTTP/1.1"); 
    client.print("Host: "); 
    client.println(server); // Host header is MANDATORY for HTTP/1.1
    client.println("Accept: application/json");
    // NOTE: X-Auth-Token header is usually NOT needed for public lab setups or if security is not enforced.
    // client.print("X-Auth-Token: "); 
    // client.println("59CO7vvjVhv7_06oJTmTinV5t2A73q0IDLdcaib3_K7hRk6rF2lli-DZUKIljnU9T2mIY9DtwGxFKeuQYqVtGg");
    client.println("Connection: close");
    client.println(); // Blank line terminates headers
  } else {
    Serial.println("Connection failed!");
    return "";
  }
  
  // --- Read Server Response ---
  String buffer;
  // Set a timeout for reading data
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 10000) { // Wait up to 10 seconds for data
      Serial.println(">>> Client Timeout!");
      client.stop();
      return "";
    }
  }

  // Read all bytes coming from the server
  while (client.connected() || client.available()) {
    if (client.available()) {
      char c = client.read();
      buffer += c;
    }
  }
  client.stop();
  return buffer;
}


/**
 * Prints the current WiFi connection status to the Serial Monitor.
 **/
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}