#include "Buttons.h"

Buttons::Buttons(uint8_t b1, uint8_t b2, uint8_t b3) {
  buttons_[0] = {b1, false, false, false, 0};
  buttons_[1] = {b2, false, false, false, 0};
  buttons_[2] = {b3, false, false, false, 0};
}

void Buttons::begin() {
  for (ButtonState& button : buttons_) {
    pinMode(button.pin, INPUT_PULLUP);
    button.stablePressed = false;
    button.lastReadPressed = false;
    button.shortPressLatched = false;
    button.lastTransitionMs = millis();
  }
}

void Buttons::update() {
  const uint32_t nowMs = millis();
  for (ButtonState& button : buttons_) {
    const bool rawPressed = digitalRead(button.pin) == LOW;
    if (rawPressed != button.lastReadPressed) {
      button.lastReadPressed = rawPressed;
      button.lastTransitionMs = nowMs;
    }
    if ((nowMs - button.lastTransitionMs) >= kDebounceMs && button.stablePressed != rawPressed) {
      const bool wasPressed = button.stablePressed;
      button.stablePressed = rawPressed;
      if (wasPressed && !button.stablePressed) {
        button.shortPressLatched = true;
      }
    }
  }
}

bool Buttons::wasShortPressed(ButtonId id) {
  ButtonState& button = buttons_[static_cast<uint8_t>(id)];
  if (button.shortPressLatched) {
    button.shortPressLatched = false;
    return true;
  }
  return false;
}
