#pragma once
#include <Arduino.h>

// KY-040 style rotary encoder with push switch.
// CLK/DT must be on external-interrupt pins (D0, D1, D2, D3 or D7 on the 32U4).
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
  static void isr();
  static volatile int16_t sSteps;
  static volatile uint8_t sState;
  static uint8_t sPinClk;
  static uint8_t sPinDt;

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
