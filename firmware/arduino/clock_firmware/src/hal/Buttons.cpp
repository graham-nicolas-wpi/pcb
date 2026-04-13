#include "Buttons.h"

Buttons::Buttons(uint8_t b1, uint8_t b2, uint8_t b3) {
  mButtons[0] = {b1, false, false, false, 0};
  mButtons[1] = {b2, false, false, false, 0};
  mButtons[2] = {b3, false, false, false, 0};
}

void Buttons::begin() {
  for (auto& b : mButtons) {
    pinMode(b.pin, INPUT_PULLUP);
    b.stablePressed = false;
    b.lastReadPressed = false;
    b.shortPressLatched = false;
    b.lastTransitionMs = millis();
  }
}

void Buttons::update() {
  const uint32_t now = millis();
  for (auto& b : mButtons) {
    const bool rawPressed = digitalRead(b.pin) == LOW;
    if (rawPressed != b.lastReadPressed) {
      b.lastReadPressed = rawPressed;
      b.lastTransitionMs = now;
    }
    if ((now - b.lastTransitionMs) >= kDebounceMs && b.stablePressed != rawPressed) {
      const bool wasPressed = b.stablePressed;
      b.stablePressed = rawPressed;
      if (wasPressed && !b.stablePressed) b.shortPressLatched = true;
    }
  }
}

bool Buttons::wasShortPressed(ButtonId id) {
  ButtonState& b = mButtons[static_cast<uint8_t>(id)];
  if (b.shortPressLatched) {
    b.shortPressLatched = false;
    return true;
  }
  return false;
}
