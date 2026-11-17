#include "RotaryEncoder.h"

volatile int16_t RotaryEncoder::sSteps = 0;
volatile uint8_t RotaryEncoder::sState = 0;
uint8_t RotaryEncoder::sPinClk = 0;
uint8_t RotaryEncoder::sPinDt = 0;

// Gray-code transition table: index = (previous AB << 2) | current AB.
// Invalid (bounced / skipped) transitions count as 0.
static const int8_t kQuadTable[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

RotaryEncoder::RotaryEncoder(uint8_t pinClk, uint8_t pinDt, uint8_t pinSw)
    : pinSw_(pinSw),
      rawPressed_(false),
      stablePressed_(false),
      lastChangeMs_(0),
      pressStartMs_(0),
      holdFired_(false),
      click_(false),
      longPress_(false),
      hold_(false) {
  sPinClk = pinClk;
  sPinDt = pinDt;
}

void RotaryEncoder::begin() {
  pinMode(sPinClk, INPUT_PULLUP);
  pinMode(sPinDt, INPUT_PULLUP);
  pinMode(pinSw_, INPUT_PULLUP);  // the KY-040 has no pull-up fitted on SW
  sState = static_cast<uint8_t>((digitalRead(sPinClk) << 1) | digitalRead(sPinDt));
  sSteps = 0;
  attachInterrupt(digitalPinToInterrupt(sPinClk), isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(sPinDt), isr, CHANGE);
  lastChangeMs_ = millis();
}

void RotaryEncoder::isr() {
  const uint8_t ab = static_cast<uint8_t>((digitalRead(sPinClk) << 1) | digitalRead(sPinDt));
  const uint8_t idx = static_cast<uint8_t>(((sState << 2) | ab) & 0x0F);
  sState = ab;
  sSteps += kQuadTable[idx];
}

int8_t RotaryEncoder::takeDetents() {
  noInterrupts();
  const int16_t steps = sSteps;
  const int16_t detents = steps / static_cast<int16_t>(kStepsPerDetent);
  sSteps = steps - detents * static_cast<int16_t>(kStepsPerDetent);
  interrupts();
  int16_t d = kReverse ? -detents : detents;
  if (d > 100) d = 100;
  if (d < -100) d = -100;
  return static_cast<int8_t>(d);
}

void RotaryEncoder::update() {
  const uint32_t now = millis();
  const bool raw = digitalRead(pinSw_) == LOW;
  if (raw != rawPressed_) {
    rawPressed_ = raw;
    lastChangeMs_ = now;
  }
  if ((now - lastChangeMs_) >= kDebounceMs && raw != stablePressed_) {
    stablePressed_ = raw;
    if (stablePressed_) {
      pressStartMs_ = now;
      holdFired_ = false;
    } else if (!holdFired_) {
      if ((now - pressStartMs_) >= kLongPressMs) {
        longPress_ = true;
      } else {
        click_ = true;
      }
    }
  }
  if (stablePressed_ && !holdFired_ && (now - pressStartMs_) >= kHoldMs) {
    holdFired_ = true;
    hold_ = true;
  }
}

bool RotaryEncoder::takeClick() {
  const bool v = click_;
  click_ = false;
  return v;
}

bool RotaryEncoder::takeLongPress() {
  const bool v = longPress_;
  longPress_ = false;
  return v;
}

bool RotaryEncoder::takeHold() {
  const bool v = hold_;
  hold_ = false;
  return v;
}
