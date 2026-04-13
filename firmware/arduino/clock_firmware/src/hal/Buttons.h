#pragma once
#include <Arduino.h>

class Buttons {
public:
  enum class ButtonId : uint8_t { BTN1 = 0, BTN2 = 1, BTN3 = 2 };
  Buttons(uint8_t b1, uint8_t b2, uint8_t b3);
  void begin();
  void update();
  bool wasShortPressed(ButtonId id);
private:
  struct ButtonState {
    uint8_t pin;
    bool stablePressed;
    bool lastReadPressed;
    bool shortPressLatched;
    uint32_t lastTransitionMs;
  };
  static constexpr uint16_t kDebounceMs = 25;
  ButtonState mButtons[3];
};
