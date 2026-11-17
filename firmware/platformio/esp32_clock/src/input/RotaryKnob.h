#pragma once

#include <Arduino.h>

// KY-040 rotary encoder with push switch. Quadrature decoding runs in a pin
// interrupt so fast spins are not lost while WiFi is busy; button timing is
// done in poll().
class RotaryKnob {
 public:
  enum class Press : uint8_t { NONE = 0, CLICK, LONG, HOLD };

  // KY-040 gives 4 quadrature steps per detent (one click).
  static constexpr int8_t kStepsPerDetent = 4;
  static constexpr uint32_t kDebounceMs = 25;
  static constexpr uint32_t kLongPressMs = 800;   // release after this = LONG
  static constexpr uint32_t kHoldMs = 2000;       // still held after this = HOLD
  // Flip if turning clockwise makes things go down (or swap CLK and DT wires).
  static constexpr bool kReverse = false;

  void begin(uint8_t clkPin, uint8_t dtPin, uint8_t swPin);

  // Call often. Returns detents turned since last call (+ = clockwise) and
  // the press event that finished, if any.
  int16_t takeDetents();
  Press poll(uint32_t nowMs);
  bool isHeld() const { return pressed_; }

 private:
  static void IRAM_ATTR isr(void* arg);
  void IRAM_ATTR handleEdge();

  uint8_t clkPin_ = 0;
  uint8_t dtPin_ = 0;
  uint8_t swPin_ = 0;
  volatile uint8_t lastState_ = 0;
  volatile int16_t steps_ = 0;
  int16_t residual_ = 0;

  bool rawPressed_ = false;
  bool pressed_ = false;
  bool holdFired_ = false;
  uint32_t rawChangedMs_ = 0;
  uint32_t pressedAtMs_ = 0;
};
