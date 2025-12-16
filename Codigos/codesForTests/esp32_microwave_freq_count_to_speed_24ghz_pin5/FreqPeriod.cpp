#include "FreqPeriod.h"
FreqPeriodClass FreqPeriod;

void FreqPeriodClass::begin(uint8_t inputPin, uint8_t feedbackPin) {
  _inputPin = inputPin;
  _feedbackPin = feedbackPin;
  _captureReady = false;
  
  #if defined(ARDUINO_ARCH_AVR)
    _beginAVR();
  #elif defined(ARDUINO_ARCH_ESP32)
    _beginESP32();
  #elif defined(ARDUINO_ARCH_STM32)
    _beginSTM32();
  #else
    #error "Unsupported platform for FreqPeriod library"
  #endif
}

bool FreqPeriodClass::available() {
  return _captureReady;
}

unsigned long FreqPeriodClass::getPeriod() {
  _captureReady = false;
  return _period;
}

// AVR-specific implementation
#if defined(ARDUINO_ARCH_AVR)
void FreqPeriodClass::_beginAVR() {
  pinMode(_feedbackPin, OUTPUT);
  
  TCCR1A = 0;
  TCCR1B = 0;
  
  // Configure analog comparator
  ADCSRB &= ~(1 << ACME);
  ACSR &= ~(1 << ACD);    // Enable analog comparator
  ACSR |= (1 << ACIE);    // Enable comparator interrupt
  ACSR |= (1 << ACIC);    // Enable comparator input capture
  
  // Configure falling edge detection initially
  ACSR &= ~(1 << ACIS0);
  ACSR |= (1 << ACIS1);
  
  // Set Timer1 prescaler to 1 (no prescaling)
  TCCR1B |= (1 << CS10);
  TCCR1B |= (1 << ICNC1);  // Input noise canceler on
  TCCR1B &= ~(1 << ICES1); // Capture on falling edge
  
  // Disable digital input buffers
  DIDR1 |= (1 << AIN0D);
  DIDR1 |= (1 << AIN1D);
  
  // Enable Timer1 overflow interrupt
  TIMSK1 |= (1 << TOIE1);
}

// Timer1 overflow interrupt
ISR(TIMER1_OVF_vect) {
  FreqPeriod._overflowCount++;
}

// Analog comparator interrupt
ISR(ANALOG_COMP_vect) {
  if (!(ACSR & (1 << ACIS0))) {  // Falling edge
    digitalWrite(FreqPeriod._feedbackPin, LOW);
    ACSR |= (1 << ACIS0);        // Next detection on rising edge
    
    // Calculate period
    unsigned long currentCapture = ICR1 + (FreqPeriod._overflowCount * 0x10000UL);
    FreqPeriod._period = currentCapture - FreqPeriod._captureValue;
    FreqPeriod._captureValue = currentCapture;
    FreqPeriod._overflowCount = 0;
    FreqPeriod._captureReady = true;
  } else {  // Rising edge
    ACSR &= ~(1 << ACIS0);       // Next detection on falling edge
    digitalWrite(FreqPeriod._feedbackPin, HIGH);
  }
}
#endif

// ESP32-specific implementation
#if defined(ARDUINO_ARCH_ESP32)
void FreqPeriodClass::_beginESP32() {
  pinMode(_feedbackPin, OUTPUT);
  
  // Configure GPIO interrupt for input pin
  attachInterrupt(digitalPinToInterrupt(_inputPin), []() {
    static unsigned long lastTime = 0;
    unsigned long currentTime = micros();
    
    if (digitalRead(FreqPeriod._inputPin)) {
      // Rising edge
      digitalWrite(FreqPeriod._feedbackPin, HIGH);
    } else {
      // Falling edge
      digitalWrite(FreqPeriod._feedbackPin, LOW);
      FreqPeriod._period = currentTime - lastTime;
      FreqPeriod._captureReady = true;
    }
    lastTime = currentTime;
  }, CHANGE);
}
#endif

// STM32-specific implementation
#if defined(ARDUINO_ARCH_STM32)
void FreqPeriodClass::_beginSTM32() {
  pinMode(_feedbackPin, OUTPUT);
  
  _timer = new HardwareTimer(TIM1);
  _timer->setMode(1, TIMER_INPUT_CAPTURE_RISING, _inputPin);
  _timer->attachInterrupt([]() {
    static unsigned long lastCapture = 0;
    unsigned long currentCapture = _timer->getCaptureCompare(1);
    
    if (digitalRead(FreqPeriod._inputPin)) {
      // Rising edge
      digitalWrite(FreqPeriod._feedbackPin, HIGH);
    } else {
      // Falling edge
      digitalWrite(FreqPeriod._feedbackPin, LOW);
      FreqPeriod._period = currentCapture - lastCapture;
      FreqPeriod._captureReady = true;
    }
    lastCapture = currentCapture;
  });
  _timer->resume();
}
#endif
