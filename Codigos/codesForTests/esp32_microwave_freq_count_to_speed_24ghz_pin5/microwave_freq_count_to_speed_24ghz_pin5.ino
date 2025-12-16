#include <Arduino.h>
#include "FreqPeriod.h"  // Make sure you're using the modified header

// Create an instance of the FreqPeriod class

extern FreqPeriodClass FreqPeriod;
double lfrq;
long int pp;

void setup() {
  Serial.begin(9600);
  
  // Initialize with input pin (connect radar output to pin 5)
  FreqPeriod.begin(5);  // Changed from FreqPeriod::begin()
  
  Serial.println("CDM324 Doppler Radar - Speed Measurement");
}

void loop() {
  if (FreqPeriod.available()) {  // Check if new data is available
    pp = FreqPeriod.getPeriod(); // Changed from FreqPeriod::getPeriod()
    
    if (pp > 0) {
      Serial.print("Period: ");
      Serial.print(pp);
      Serial.print(" ticks / Frequency: ");
      
      // Calculate frequency (adjust formula as needed for your radar module)
      lfrq = 59000.00 / pp;  // Example conversion factor
      
      Serial.print(lfrq);
      Serial.print(" Hz / Speed: ");
      Serial.print(lfrq / 31.36);  // Standard Doppler conversion (Hz to MPH)
      Serial.println(" MPH");
    }
  }
  delay(250);
}