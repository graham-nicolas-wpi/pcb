#pragma once
#include <Arduino.h>

// KY-040 style rotary encoder with push switch.
// CLK must be D0 (PD2/INT2) and DT must be D1 (PD3/INT3): the ISR reads PIND
// directly and sets up INT2/INT3 itself (no attachInterrupt, to save flash).
// Only one instance is supported (the ISR uses static state).
class RotaryEncoder {
 public:
  RotaryEncoder(uint8_t pinClk, uint8_t pinDt, uint8_t pinSw);
  void begin();
  void update();  // call every loop: debounces the switch

  int8_t takeDetents();  // net clicks of the knob since the last call, + = clockwise
  bool takeClick();      // short press, released before kLongPressMs
  bool takeLongPress();  // held kLongPressMs..kHoldMs, then released
  bool takeHold();       // held kHoldMs (fires while still held)

  static constexpr uint8_t kStepsPerDetent = 4;    // KY-040: one full quadrature cycle per click
  static constexpr bool kReverse = false;          // flip if clockwise turns the wrong way
  static constexpr uint16_t kDebounceMs = 20;
  static constexpr uint16_t kLongPressMs = 600;
  static constexpr uint16_t kHoldMs = 2000;

 private:
  friend void rotaryEncoderIsr();
  static volatile int16_t sSteps;
  static volatile uint8_t sState;
  uint8_t pinClk_;
  uint8_t pinDt_;

  uint8_t pinSw_;
  bool rawPressed_;
  bool stablePressed_;
  uint32_t lastChangeMs_;
  uint32_t pressStartMs_;
  bool holdFired_;
  bool click_;
  bool longPress_;
  bool hold_;
};
