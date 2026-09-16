#ifndef FreqPeriod_h
#define FreqPeriod_h

#include <Arduino.h>

// Platform detection
#if defined(ARDUINO_ARCH_AVR)
  #include <avr/interrupt.h>
#elif defined(ARDUINO_ARCH_ESP32)
  #include "driver/timer.h"
  #include "driver/gptimer.h"
#elif defined(ARDUINO_ARCH_STM32)
  #include <HardwareTimer.h>
#endif

class FreqPeriodClass {
  public:
    void begin(uint8_t inputPin, uint8_t feedbackPin = 5);
    unsigned long getPeriod();
    bool available();
    
  private:
    volatile bool _captureReady;
    volatile unsigned long _period;
    uint8_t _inputPin;
    uint8_t _feedbackPin;
    
    // Platform-specific implementations
    #if defined(ARDUINO_ARCH_AVR)
      volatile unsigned int _captureValue;
      volatile unsigned int _overflowCount;
      void _beginAVR();
    #elif defined(ARDUINO_ARCH_ESP32)
      void _beginESP32();
      static bool _timerISR(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data);
    #elif defined(ARDUINO_ARCH_STM32)
      HardwareTimer *_timer;
      void _beginSTM32();
    #endif
};

extern FreqPeriodClass FreqPeriod;

#endif
