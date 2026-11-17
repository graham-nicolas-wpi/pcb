#include "RotaryEncoder.h"

#include <avr/interrupt.h>

volatile int16_t RotaryEncoder::sSteps = 0;
volatile uint8_t RotaryEncoder::sState = 0;

// Gray-code transition table: index = (previous AB << 2) | current AB.
// Invalid (bounced / skipped) transitions count as 0.
static const int8_t kQuadTable[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

RotaryEncoder::RotaryEncoder(uint8_t pinClk, uint8_t pinDt, uint8_t pinSw)
    : pinClk_(pinClk),
      pinDt_(pinDt),
      pinSw_(pinSw),
      rawPressed_(false),
      stablePressed_(false),
      lastChangeMs_(0),
      pressStartMs_(0),
      holdFired_(false),
      click_(false),
      longPress_(false),
      hold_(false) {}

// A = CLK on PD2, B = DT on PD3.
static inline uint8_t readAB() {
  const uint8_t pind = PIND;
  return static_cast<uint8_t>((((pind >> 2) & 1U) << 1) | ((pind >> 3) & 1U));
}

void rotaryEncoderIsr() {
  const uint8_t ab = readAB();
  const uint8_t idx = static_cast<uint8_t>(((RotaryEncoder::sState << 2) | ab) & 0x0F);
  RotaryEncoder::sState = ab;
  RotaryEncoder::sSteps += kQuadTable[idx];
}

ISR(INT2_vect) { rotaryEncoderIsr(); }
ISR(INT3_vect) { rotaryEncoderIsr(); }

void RotaryEncoder::begin() {
  pinMode(pinClk_, INPUT_PULLUP);
  pinMode(pinDt_, INPUT_PULLUP);
  pinMode(pinSw_, INPUT_PULLUP);  // the KY-040 has no pull-up fitted on SW
  sState = readAB();
  sSteps = 0;
  // INT2 / INT3 on any logical change (ISCn1:0 = 01), clear stale flags, enable.
  EICRA = static_cast<uint8_t>((EICRA & ~((1 << ISC21) | (1 << ISC31))) | (1 << ISC20) | (1 << ISC30));
  EIFR = (1 << INTF2) | (1 << INTF3);
  EIMSK |= (1 << INT2) | (1 << INT3);
  lastChangeMs_ = millis();
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
