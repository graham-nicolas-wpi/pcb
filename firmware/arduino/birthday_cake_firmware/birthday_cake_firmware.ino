#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

namespace Pins {
constexpr uint8_t kPixels = 5;
// ATmega32U4 pad 1 is PE6, which is Arduino Leonardo digital pin 7.
constexpr uint8_t kButtons[] = {7};
constexpr uint8_t kButtonCount = sizeof(kButtons) / sizeof(kButtons[0]);
constexpr uint8_t kBuzzer = 13;
}

namespace Leds {
constexpr uint8_t kCandleCount = 9;
constexpr uint8_t kSparkleCount = 5;
constexpr uint8_t kTotalCount = 31;

// Update these arrays if the physical LED chain is wired in a different order.
constexpr uint8_t kCandles[kCandleCount] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr uint8_t kSparkles[kSparkleCount] = {9, 10, 11, 12, 13};
}

namespace Timing {
constexpr uint16_t kDebounceMs = 25;
constexpr uint16_t kCandleStepMs = 120;
constexpr uint16_t kTwinkleStepMs = 55;
constexpr uint16_t kCandleFlickerMs = 65;
constexpr uint16_t kRestMs = 35;
constexpr uint16_t kFinaleFlickerMs = 2200;
constexpr uint16_t kConfettiFadeMs = 1400;
}

namespace Colors {
constexpr uint8_t kBrightness = 110;
constexpr uint32_t kCandleFlame = 0xFF7A12;
constexpr uint8_t kConfettiBrightness = 135;
}

namespace Buzzer {
// The buzzer behaves like a passive piezo: DC only clicks, so make our own tone.
constexpr bool kUseSoftwareSquareWave = true;
constexpr bool kUseMelodyPitch = true;
constexpr uint16_t kFixedBuzzFrequencyHz = 900;
constexpr uint8_t kMelodyPitchScalePercent = 200;
constexpr uint8_t kOnLevel = LOW;
constexpr uint8_t kOffLevel = HIGH;
constexpr uint8_t kDurationScalePercent = 140;
}

Adafruit_NeoPixel pixels(Leds::kTotalCount, Pins::kPixels, NEO_GRB + NEO_KHZ800);

enum class ShowState : uint8_t {
  Idle,
  LightingCandles,
  PlayingSong,
  FinaleFlicker,
  ConfettiFade,
  Done,
};

struct ButtonState {
  bool stablePressed = false;
  bool lastReadPressed = false;
  bool shortPressLatched = false;
  bool armed = false;
  uint32_t lastTransitionMs = 0;
  uint32_t releasedSinceMs = 0;
  uint8_t pin = 0;
};

struct SongNote {
  uint16_t frequency;
  uint16_t durationMs;
};

constexpr uint16_t NOTE_C4 = 262;
constexpr uint16_t NOTE_D4 = 294;
constexpr uint16_t NOTE_E4 = 330;
constexpr uint16_t NOTE_F4 = 349;
constexpr uint16_t NOTE_G4 = 392;
constexpr uint16_t NOTE_A4 = 440;
constexpr uint16_t NOTE_AS4 = 466;
constexpr uint16_t NOTE_C5 = 523;

constexpr SongNote kHappyBirthday[] = {
    {NOTE_C4, 180}, {NOTE_C4, 180}, {NOTE_D4, 360}, {NOTE_C4, 360},
    {NOTE_F4, 360}, {NOTE_E4, 720},
    {NOTE_C4, 180}, {NOTE_C4, 180}, {NOTE_D4, 360}, {NOTE_C4, 360},
    {NOTE_G4, 360}, {NOTE_F4, 720},
    {NOTE_C4, 180}, {NOTE_C4, 180}, {NOTE_C5, 360}, {NOTE_A4, 360},
    {NOTE_F4, 360}, {NOTE_E4, 360}, {NOTE_D4, 720},
    {NOTE_AS4, 180}, {NOTE_AS4, 180}, {NOTE_A4, 360}, {NOTE_F4, 360},
    {NOTE_G4, 360}, {NOTE_F4, 900},
};

constexpr uint8_t kSongLength = sizeof(kHappyBirthday) / sizeof(kHappyBirthday[0]);

ButtonState buttons[Pins::kButtonCount];
ShowState showState = ShowState::Idle;

uint8_t candlesLit = 0;
uint8_t currentNote = 0;
bool noteSounding = false;
bool buzzerOutputOn = false;
bool candleAlive[Leds::kCandleCount] = {false};
uint32_t lastCandleStepMs = 0;
uint32_t lastCandleFlickerMs = 0;
uint32_t lastTwinkleStepMs = 0;
uint32_t noteStartedMs = 0;
uint32_t finaleStartedMs = 0;
uint32_t nextCandleOutMs = 0;
uint32_t lastBuzzerToggleUs = 0;
uint16_t currentBuzzFrequencyHz = Buzzer::kFixedBuzzFrequencyHz;

uint32_t colorFromRgb(uint8_t r, uint8_t g, uint8_t b) {
  return pixels.Color(r, g, b);
}

uint32_t scaleColor(uint32_t color, uint8_t brightness) {
  const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
  const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
  const uint8_t b = static_cast<uint8_t>(color & 0xFF);
  return colorFromRgb(static_cast<uint8_t>((static_cast<uint16_t>(r) * brightness) / 255U),
                      static_cast<uint8_t>((static_cast<uint16_t>(g) * brightness) / 255U),
                      static_cast<uint8_t>((static_cast<uint16_t>(b) * brightness) / 255U));
}

uint32_t colorFromWheel(uint8_t wheelPos) {
  wheelPos = 255 - wheelPos;
  if (wheelPos < 85) {
    return colorFromRgb(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if (wheelPos < 170) {
    wheelPos -= 85;
    return colorFromRgb(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return colorFromRgb(wheelPos * 3, 255 - wheelPos * 3, 0);
}

uint16_t noteDurationMs(const SongNote& note) {
  return static_cast<uint16_t>((static_cast<uint32_t>(note.durationMs) *
                                Buzzer::kDurationScalePercent) /
                               100UL);
}

uint16_t buzzFrequencyForNote(const SongNote& note) {
  if (!Buzzer::kUseMelodyPitch) {
    return Buzzer::kFixedBuzzFrequencyHz;
  }

  return static_cast<uint16_t>((static_cast<uint32_t>(note.frequency) *
                                Buzzer::kMelodyPitchScalePercent) /
                               100UL);
}

void clearSparkles() {
  for (uint8_t i = 0; i < Leds::kSparkleCount; ++i) {
    pixels.setPixelColor(Leds::kSparkles[i], 0);
  }
}

void renderCandles() {
  for (uint8_t i = 0; i < Leds::kCandleCount; ++i) {
    const bool lit = i < candlesLit;
    candleAlive[i] = lit;
    pixels.setPixelColor(Leds::kCandles[i], lit ? Colors::kCandleFlame : 0);
  }
}

void renderCandleFlicker(uint8_t flickerAmount, uint8_t windBrightness = 255) {
  for (uint8_t i = 0; i < Leds::kCandleCount; ++i) {
    if (i >= candlesLit || !candleAlive[i]) {
      pixels.setPixelColor(Leds::kCandles[i], 0);
      continue;
    }

    const int16_t flicker = random(-static_cast<int16_t>(flickerAmount),
                                   static_cast<int16_t>(flickerAmount) + 1);
    const uint8_t r = static_cast<uint8_t>(constrain(235 + flicker, 95, 255));
    const uint8_t g = static_cast<uint8_t>(constrain(85 + flicker, 12, 155));
    const uint8_t b = static_cast<uint8_t>(constrain(4 + (flicker / 5), 0, 28));
    pixels.setPixelColor(Leds::kCandles[i], scaleColor(colorFromRgb(r, g, b), windBrightness));
  }
}

void updateCandleFlicker(uint32_t nowMs, uint8_t flickerAmount) {
  if (nowMs - lastCandleFlickerMs < Timing::kCandleFlickerMs) {
    return;
  }

  lastCandleFlickerMs = nowMs;
  renderCandleFlicker(flickerAmount);
  pixels.show();
}

void twinkleSparkles(uint32_t nowMs, uint8_t brightness) {
  if (nowMs - lastTwinkleStepMs < Timing::kTwinkleStepMs) {
    return;
  }

  lastTwinkleStepMs = nowMs;
  for (uint8_t i = 0; i < Leds::kSparkleCount; ++i) {
    const uint8_t chance = random(0, 100);
    if (chance < 25) {
      pixels.setPixelColor(Leds::kSparkles[i], 0);
    } else {
      const uint8_t hue = random(0, 255);
      pixels.setPixelColor(Leds::kSparkles[i], scaleColor(colorFromWheel(hue), brightness));
    }
  }
  pixels.show();
}

void buzzerOff() {
  if (Buzzer::kUseSoftwareSquareWave) {
    digitalWrite(Pins::kBuzzer, Buzzer::kOffLevel);
  } else {
    noTone(Pins::kBuzzer);
  }
  noteSounding = false;
  buzzerOutputOn = false;
}

void startNote(uint8_t noteIndex) {
  const SongNote& note = kHappyBirthday[noteIndex];
  currentBuzzFrequencyHz = buzzFrequencyForNote(note);
  if (Buzzer::kUseSoftwareSquareWave) {
    digitalWrite(Pins::kBuzzer, Buzzer::kOnLevel);
    buzzerOutputOn = true;
    lastBuzzerToggleUs = micros();
  } else {
    tone(Pins::kBuzzer, note.frequency);
  }
  noteSounding = true;
  noteStartedMs = millis();
}

void updateBuzzer() {
  if (!Buzzer::kUseSoftwareSquareWave || !noteSounding) {
    return;
  }

  const uint32_t halfPeriodUs = 500000UL / currentBuzzFrequencyHz;
  const uint32_t nowUs = micros();
  if (nowUs - lastBuzzerToggleUs < halfPeriodUs) {
    return;
  }

  lastBuzzerToggleUs = nowUs;
  buzzerOutputOn = !buzzerOutputOn;
  digitalWrite(Pins::kBuzzer, buzzerOutputOn ? Buzzer::kOnLevel : Buzzer::kOffLevel);
}

void startShow(uint32_t nowMs) {
  buzzerOff();
  pixels.clear();
  for (uint8_t i = 0; i < Leds::kCandleCount; ++i) {
    candleAlive[i] = false;
  }
  candlesLit = 1;
  candleAlive[0] = true;
  currentNote = 0;
  showState = ShowState::LightingCandles;
  lastCandleStepMs = nowMs;
  lastCandleFlickerMs = nowMs;
  lastTwinkleStepMs = nowMs;
  renderCandles();
  pixels.show();
}

void startSong(uint32_t nowMs) {
  showState = ShowState::PlayingSong;
  currentNote = 0;
  clearSparkles();
  renderCandleFlicker(18);
  pixels.show();
  startNote(currentNote);
}

void startFinale(uint32_t nowMs) {
  buzzerOff();
  showState = ShowState::FinaleFlicker;
  finaleStartedMs = nowMs;
  candlesLit = Leds::kCandleCount;
  for (uint8_t i = 0; i < Leds::kCandleCount; ++i) {
    candleAlive[i] = true;
  }
  lastCandleFlickerMs = 0;
  lastTwinkleStepMs = 0;
  nextCandleOutMs = nowMs + random(180, 360);
}

void startConfettiFade(uint32_t nowMs) {
  candlesLit = 0;
  clearSparkles();
  renderCandles();
  showState = ShowState::ConfettiFade;
  finaleStartedMs = nowMs;
  lastTwinkleStepMs = 0;
  pixels.show();
}

void updateButton(ButtonState& button, uint32_t nowMs) {
  const bool rawPressed = digitalRead(button.pin) == LOW;
  if (rawPressed != button.lastReadPressed) {
    button.lastReadPressed = rawPressed;
    button.lastTransitionMs = nowMs;
  }

  if ((nowMs - button.lastTransitionMs) >= Timing::kDebounceMs &&
      button.stablePressed != rawPressed) {
    button.stablePressed = rawPressed;
    if (!button.stablePressed) {
      button.releasedSinceMs = nowMs;
    }
    if (button.armed && button.stablePressed) {
      button.shortPressLatched = true;
    }
  }

  if (button.stablePressed) {
    button.releasedSinceMs = nowMs;
  } else if (!button.armed && nowMs - button.releasedSinceMs > 200) {
    button.armed = true;
  }
}

void updateButtons(uint32_t nowMs) {
  for (uint8_t i = 0; i < Pins::kButtonCount; ++i) {
    updateButton(buttons[i], nowMs);
  }
}

bool consumeButtonPress() {
  bool pressed = false;
  for (uint8_t i = 0; i < Pins::kButtonCount; ++i) {
    if (buttons[i].shortPressLatched) {
      buttons[i].shortPressLatched = false;
      pressed = true;
    }
  }
  return pressed;
}

void updateLightingCandles(uint32_t nowMs) {
  if (candlesLit >= Leds::kCandleCount) {
    startSong(nowMs);
    return;
  }

  if (nowMs - lastCandleStepMs >= Timing::kCandleStepMs) {
    ++candlesLit;
    candleAlive[candlesLit - 1] = true;
    lastCandleStepMs = nowMs;
    renderCandles();
    pixels.show();
  }
}

void updateSong(uint32_t nowMs) {
  updateCandleFlicker(nowMs, 18);
  twinkleSparkles(nowMs, Colors::kConfettiBrightness);

  const SongNote& note = kHappyBirthday[currentNote];
  const uint16_t durationMs = noteDurationMs(note);

  if (noteSounding && nowMs - noteStartedMs >= durationMs) {
    buzzerOff();
  }

  if (nowMs - noteStartedMs < durationMs + Timing::kRestMs) {
    return;
  }

  ++currentNote;
  if (currentNote >= kSongLength) {
    startFinale(nowMs);
    return;
  }

  startNote(currentNote);
}

uint8_t countAliveCandles() {
  uint8_t alive = 0;
  for (uint8_t i = 0; i < Leds::kCandleCount; ++i) {
    if (candleAlive[i]) {
      ++alive;
    }
  }
  return alive;
}

void blowOutRandomCandle() {
  const uint8_t alive = countAliveCandles();
  if (alive == 0) {
    return;
  }

  uint8_t target = random(0, alive);
  for (uint8_t i = 0; i < Leds::kCandleCount; ++i) {
    if (!candleAlive[i]) {
      continue;
    }
    if (target == 0) {
      candleAlive[i] = false;
      return;
    }
    --target;
  }
}

void updateFinaleFlicker(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - finaleStartedMs;
  const uint8_t alive = countAliveCandles();

  if (alive > 0 && nowMs >= nextCandleOutMs) {
    blowOutRandomCandle();
    const uint16_t minDelay = elapsed < 900 ? 210 : 110;
    const uint16_t maxDelay = elapsed < 900 ? 430 : 260;
    nextCandleOutMs = nowMs + random(minDelay, maxDelay);
  }

  if (nowMs - lastCandleFlickerMs >= Timing::kCandleFlickerMs) {
    lastCandleFlickerMs = nowMs;
    const uint8_t windBase =
        static_cast<uint8_t>(constrain(230 - static_cast<int16_t>(elapsed / 8), 70, 230));
    const uint8_t windPulse = random(35, 125);
    const uint8_t windBrightness =
        static_cast<uint8_t>(constrain(static_cast<int16_t>(windBase) + windPulse - 80, 20, 255));
    renderCandleFlicker(105, windBrightness);
    pixels.show();
  }

  twinkleSparkles(nowMs, Colors::kConfettiBrightness);

  if (elapsed >= Timing::kFinaleFlickerMs || countAliveCandles() == 0) {
    startConfettiFade(nowMs);
  }
}

void updateConfettiFade(uint32_t nowMs) {
  const uint32_t elapsed = nowMs - finaleStartedMs;
  if (elapsed >= Timing::kConfettiFadeMs) {
    pixels.clear();
    pixels.show();
    showState = ShowState::Done;
    return;
  }

  const uint8_t brightness =
      static_cast<uint8_t>((static_cast<uint32_t>(Colors::kConfettiBrightness) *
                            (Timing::kConfettiFadeMs - elapsed)) /
                           Timing::kConfettiFadeMs);
  twinkleSparkles(nowMs, brightness);
}

void setup() {
  for (uint8_t i = 0; i < Pins::kButtonCount; ++i) {
    buttons[i].pin = Pins::kButtons[i];
    pinMode(buttons[i].pin, INPUT_PULLUP);
    buttons[i].lastReadPressed = digitalRead(buttons[i].pin) == LOW;
    buttons[i].stablePressed = buttons[i].lastReadPressed;
    buttons[i].lastTransitionMs = millis();
    buttons[i].releasedSinceMs = millis();
  }
  digitalWrite(Pins::kBuzzer, Buzzer::kOffLevel);
  pinMode(Pins::kBuzzer, OUTPUT);
  buzzerOff();

  randomSeed(analogRead(A0));
  pixels.begin();
  pixels.setBrightness(Colors::kBrightness);
  pixels.clear();
  pixels.show();
}

void loop() {
  const uint32_t nowMs = millis();
  updateBuzzer();
  updateButtons(nowMs);

  if (consumeButtonPress()) {
    startShow(nowMs);
  }

  switch (showState) {
    case ShowState::Idle:
    case ShowState::Done:
      break;
    case ShowState::LightingCandles:
      updateLightingCandles(nowMs);
      break;
    case ShowState::PlayingSong:
      updateSong(nowMs);
      break;
    case ShowState::FinaleFlicker:
      updateFinaleFlicker(nowMs);
      break;
    case ShowState::ConfettiFade:
      updateConfettiFade(nowMs);
      break;
  }
}
