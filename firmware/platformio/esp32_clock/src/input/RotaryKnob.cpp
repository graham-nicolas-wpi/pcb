#include "RotaryKnob.h"

namespace {
// Gray-code transition table: index = (old << 2) | new.
const int8_t kTransition[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
portMUX_TYPE gKnobMux = portMUX_INITIALIZER_UNLOCKED;
}

void RotaryKnob::begin(uint8_t clkPin, uint8_t dtPin, uint8_t swPin) {
  clkPin_ = clkPin;
  dtPin_ = dtPin;
  swPin_ = swPin;
  // The KY-040 board has pull-ups on CLK/DT but usually not on SW.
  pinMode(clkPin_, INPUT_PULLUP);
  pinMode(dtPin_, INPUT_PULLUP);
  pinMode(swPin_, INPUT_PULLUP);
  lastState_ = (digitalRead(clkPin_) << 1) | digitalRead(dtPin_);
  attachInterruptArg(digitalPinToInterrupt(clkPin_), isr, this, CHANGE);
  attachInterruptArg(digitalPinToInterrupt(dtPin_), isr, this, CHANGE);
}

void IRAM_ATTR RotaryKnob::isr(void* arg) { static_cast<RotaryKnob*>(arg)->handleEdge(); }

void IRAM_ATTR RotaryKnob::handleEdge() {
  const uint8_t state = (digitalRead(clkPin_) << 1) | digitalRead(dtPin_);
  portENTER_CRITICAL_ISR(&gKnobMux);
  steps_ += kTransition[(lastState_ << 2) | state];
  lastState_ = state;
  portEXIT_CRITICAL_ISR(&gKnobMux);
}

int16_t RotaryKnob::takeDetents() {
  portENTER_CRITICAL(&gKnobMux);
  residual_ += steps_;
  steps_ = 0;
  portEXIT_CRITICAL(&gKnobMux);
  const int16_t detents = residual_ / kStepsPerDetent;
  residual_ -= detents * kStepsPerDetent;
  return kReverse ? -detents : detents;
}

RotaryKnob::Press RotaryKnob::poll(uint32_t nowMs) {
  const bool raw = digitalRead(swPin_) == LOW;
  if (raw != rawPressed_) {
    rawPressed_ = raw;
    rawChangedMs_ = nowMs;
  }
  if (rawPressed_ != pressed_ && (nowMs - rawChangedMs_) >= kDebounceMs) {
    pressed_ = rawPressed_;
    if (pressed_) {
      pressedAtMs_ = nowMs;
      holdFired_ = false;
    } else if (!holdFired_) {
      return (nowMs - pressedAtMs_) >= kLongPressMs ? Press::LONG : Press::CLICK;
    }
  }
  if (pressed_ && !holdFired_ && (nowMs - pressedAtMs_) >= kHoldMs) {
    holdFired_ = true;
    return Press::HOLD;
  }
  return Press::NONE;
}
