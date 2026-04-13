#include <Adafruit_NeoPixel.h>
#include <RTClib.h>

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "LogicalIds.h"
#include "LedMap.h"
#include "PixelDriver_NeoPixel.h"
#include "LogicalDisplay.h"
#include "Buttons.h"
#include "CalibrationController.h"

namespace Pins {
  static constexpr uint8_t kPixels = 5;
  static constexpr uint8_t kBtn1 = 8;
  static constexpr uint8_t kBtn2 = 9;
  static constexpr uint8_t kBtn3 = 10;
}

namespace Timing {
  static constexpr uint32_t kClockRefreshMs = 100;
  static constexpr uint32_t kSerialLineMax = 96;
  static constexpr uint32_t kButtonLongPressMs = 800;
}


enum class ClockMode : uint8_t {
  CLOCK = 0,
  CLOCK_SECONDS,
  TIMER,
  STOPWATCH,
  COLOR_DEMO,
};

enum class HourMode : uint8_t {
  H24 = 0,
  H12,
};

enum class SecondsDisplayMode : uint8_t {
  BLINK = 0,
  ON,
  OFF,
};


struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct PersistedSettings {
  uint32_t magic;
  uint8_t version;
  uint8_t brightness;
  uint8_t mode;
  uint8_t hourMode;
  uint8_t secondsMode;
  uint8_t colonBrightness;
  uint8_t rainbowClockMode;
  uint32_t timerPresetSeconds;
  RgbColor digit1Color;
  RgbColor digit2Color;
  RgbColor digit3Color;
  RgbColor digit4Color;
  RgbColor accentColor;
  RgbColor secondsColor;
  RgbColor timerColor;
  uint16_t checksum;
};

static constexpr uint32_t kSettingsMagic = 0x434C4B31UL; // "CLK1"
static constexpr uint8_t kSettingsVersion = 2;
static constexpr int kSettingsEepromAddr = 128;

static uint32_t toColor(const RgbColor& c);
static uint32_t toColorScaled(const RgbColor& c, uint8_t scale);
static uint32_t toDisplayColor(const RgbColor& c);
static RgbColor colorFromHue(uint8_t hue);
static void renderActiveMode();
static void renderClock(bool showSeconds);
static void renderTimer();
static void renderStopwatch();
static void renderColorDemo();
static void processClockCommand(const String& line);
static void sendRtcRead();
static bool parseRtcSet(const String& line, DateTime& out);
static bool parseIntToken(const String& text, int& value);
static bool startsWithCommand(const String& line, const char* command);
static void replyOK(const String& text);
static void replyERR(const String& text);
static void resetTimerToPreset();
static uint32_t getElapsedWhileRunning(uint32_t baseElapsed, uint32_t startMs);
static uint16_t checksum16(const uint8_t* data, size_t n);
static void saveSettings();
static bool loadSettings();

PixelDriver_NeoPixel gPixels(Pins::kPixels, kNumPixels);
LedMap gMap;
LogicalDisplay gDisplay(gPixels, gMap);
Buttons gButtons(Pins::kBtn1, Pins::kBtn2, Pins::kBtn3);
CalibrationController gCal(gPixels, gMap, gDisplay);
RTC_DS3231 gRtc;


static bool gRtcPresent = false;
static bool gRtcLostPower = false;
static ClockMode gMode = ClockMode::CLOCK;
static uint32_t gLastRenderMs = 0;
static uint32_t gColorDemoStep = 0;
static HourMode gHourMode = HourMode::H24;
static SecondsDisplayMode gSecondsMode = SecondsDisplayMode::BLINK;

static RgbColor gDigit1Color = {255, 120, 40};
static RgbColor gDigit2Color = {255, 120, 40};
static RgbColor gDigit3Color = {255, 120, 40};
static RgbColor gDigit4Color = {255, 120, 40};
static RgbColor gAccentColor = {0, 120, 255};
static RgbColor gSecondsColor = {255, 255, 255};
static RgbColor gTimerColor = {80, 255, 120};
static uint8_t gBrightness = 64;
static uint8_t gColonBrightness = 255;
static bool gRainbowClockMode = false;

static uint32_t gTimerPresetSeconds = 5 * 60;
static uint32_t gTimerRemainingSeconds = 5 * 60;
static bool gTimerRunning = false;
static uint32_t gTimerLastTickMs = 0;

static bool gStopwatchRunning = false;
static uint32_t gStopwatchAccumulatedMs = 0;
static uint32_t gStopwatchStartMs = 0;

static String gSerialLine;

static bool gSettingsDirty = false;
static uint32_t gLastSettingsSaveMs = 0;

static uint32_t toColor(const RgbColor& c) {
  return gPixels.color(c.r, c.g, c.b);
}

static uint32_t toColorScaled(const RgbColor& c, uint8_t scale) {
  const uint16_t r = ((uint16_t)c.r * scale) / 255U;
  const uint16_t g = ((uint16_t)c.g * scale) / 255U;
  const uint16_t b = ((uint16_t)c.b * scale) / 255U;
  return gPixels.color((uint8_t)r, (uint8_t)g, (uint8_t)b);
}

static uint32_t toDisplayColor(const RgbColor& c) {
  return toColorScaled(c, gBrightness);
}

static RgbColor colorFromHue(uint8_t hue) {
  const uint8_t region = hue / 43;
  const uint8_t remainder = (hue - (region * 43)) * 6;
  const uint8_t q = 255 - remainder;
  const uint8_t t = remainder;

  switch (region) {
    case 0: return {255, t, 0};
    case 1: return {q, 255, 0};
    case 2: return {0, 255, t};
    case 3: return {0, q, 255};
    case 4: return {t, 0, 255};
    default: return {255, 0, q};
  }
}

static uint16_t checksum16(const uint8_t* data, size_t n) {
  uint16_t s = 0;
  for (size_t i = 0; i < n; ++i) {
    s = (uint16_t)(s + data[i]);
  }
  return s;
}

static void saveSettings() {
  PersistedSettings settings;
  settings.magic = kSettingsMagic;
  settings.version = kSettingsVersion;
  settings.brightness = gBrightness;
  settings.mode = static_cast<uint8_t>(gMode);
  settings.hourMode = static_cast<uint8_t>(gHourMode);
  settings.secondsMode = static_cast<uint8_t>(gSecondsMode);
  settings.colonBrightness = gColonBrightness;
  settings.rainbowClockMode = gRainbowClockMode ? 1 : 0;
  settings.timerPresetSeconds = gTimerPresetSeconds;
  settings.digit1Color = gDigit1Color;
  settings.digit2Color = gDigit2Color;
  settings.digit3Color = gDigit3Color;
  settings.digit4Color = gDigit4Color;
  settings.accentColor = gAccentColor;
  settings.secondsColor = gSecondsColor;
  settings.timerColor = gTimerColor;
  settings.checksum = 0;
  settings.checksum = checksum16(reinterpret_cast<const uint8_t*>(&settings), sizeof(PersistedSettings) - sizeof(uint16_t));

  EEPROM.put(kSettingsEepromAddr, settings);
  gSettingsDirty = false;
  gLastSettingsSaveMs = millis();
}

static bool loadSettings() {
  PersistedSettings settings;
  EEPROM.get(kSettingsEepromAddr, settings);

  if (settings.magic != kSettingsMagic) return false;
  if (settings.version != kSettingsVersion) return false;

  const uint16_t expected = checksum16(reinterpret_cast<const uint8_t*>(&settings), sizeof(PersistedSettings) - sizeof(uint16_t));
  if (settings.checksum != expected) return false;

  if (settings.mode <= static_cast<uint8_t>(ClockMode::COLOR_DEMO)) {
    gMode = static_cast<ClockMode>(settings.mode);
  }
  if (settings.hourMode <= static_cast<uint8_t>(HourMode::H12)) {
    gHourMode = static_cast<HourMode>(settings.hourMode);
  }
  if (settings.secondsMode <= static_cast<uint8_t>(SecondsDisplayMode::OFF)) {
    gSecondsMode = static_cast<SecondsDisplayMode>(settings.secondsMode);
  }

  gTimerPresetSeconds = settings.timerPresetSeconds;
  gDigit1Color = settings.digit1Color;
  gDigit2Color = settings.digit2Color;
  gDigit3Color = settings.digit3Color;
  gDigit4Color = settings.digit4Color;
  gAccentColor = settings.accentColor;
  gSecondsColor = settings.secondsColor;
  gTimerColor = settings.timerColor;
  gColonBrightness = settings.colonBrightness;
  gRainbowClockMode = settings.rainbowClockMode != 0;
  gBrightness = constrain(settings.brightness, 1, 255);
  if (gColonBrightness == 0) gColonBrightness = 1;
  gPixels.setBrightness(255);
  resetTimerToPreset();
  return true;
}

static void clearDisplay() {
  gDisplay.clear();
}

static void renderClock(bool showSeconds) {
  DateTime now;
  if (gRtcPresent) {
    now = gRtc.now();
  } else {
    const uint32_t seconds = millis() / 1000UL;
    const uint8_t mins = (seconds / 60UL) % 60UL;
    const uint8_t hours = (seconds / 3600UL) % 24UL;
    now = DateTime(2026, 1, 1, hours, mins, seconds % 60UL);
  }

  uint8_t h = now.hour();
  const uint8_t m = now.minute();
  const uint8_t s = now.second();

  if (gHourMode == HourMode::H12) {
    h = h % 12;
    if (h == 0) h = 12;
  }

  clearDisplay();

  RgbColor d1 = gDigit1Color;
  RgbColor d2 = gDigit2Color;
  RgbColor d3 = gDigit3Color;
  RgbColor d4 = gDigit4Color;
  RgbColor colonColor = gAccentColor;

  if (gRainbowClockMode) {
    const uint8_t base = (uint8_t)(gColorDemoStep * 5U);
    d1 = colorFromHue(base + 0);
    d2 = colorFromHue(base + 32);
    d3 = colorFromHue(base + 96);
    d4 = colorFromHue(base + 160);
    colonColor = colorFromHue(base + 224);
  }

  if (!(gHourMode == HourMode::H12 && h < 10)) {
    gDisplay.renderDigit(0, (h / 10) % 10, toDisplayColor(d1));
  }
  gDisplay.renderDigit(1, h % 10, toDisplayColor(d2));
  gDisplay.renderDigit(2, (m / 10) % 10, toDisplayColor(d3));
  gDisplay.renderDigit(3, m % 10, toDisplayColor(d4));

  bool colonOn = false;
  switch (gSecondsMode) {
    case SecondsDisplayMode::BLINK:
      colonOn = (s % 2) == 0;
      break;
    case SecondsDisplayMode::ON:
      colonOn = true;
      break;
    case SecondsDisplayMode::OFF:
      colonOn = false;
      break;
  }
  gDisplay.setColon(colonOn, colonOn, toColorScaled(colonColor, gColonBrightness));

  gDisplay.setDecimal(false, 0);

  gDisplay.show();
}

static void renderTimer() {
  const uint32_t total = gTimerRemainingSeconds;
  const uint16_t mins = total / 60UL;
  const uint8_t secs = total % 60UL;

  clearDisplay();
  gDisplay.renderDigit(0, (mins / 10) % 10, toDisplayColor(gTimerColor));
  gDisplay.renderDigit(1, mins % 10, toDisplayColor(gTimerColor));
  gDisplay.renderDigit(2, (secs / 10) % 10, toDisplayColor(gTimerColor));
  gDisplay.renderDigit(3, secs % 10, toDisplayColor(gTimerColor));
  gDisplay.setColon(true, true, toColorScaled(gAccentColor, gColonBrightness));
  gDisplay.show();
}

static void renderStopwatch() {
  const uint32_t elapsedMs = getElapsedWhileRunning(gStopwatchAccumulatedMs, gStopwatchStartMs);
  const uint32_t totalSeconds = elapsedMs / 1000UL;
  const uint16_t mins = (totalSeconds / 60UL) % 100UL;
  const uint8_t secs = totalSeconds % 60UL;

  clearDisplay();
  gDisplay.renderDigit(0, (mins / 10) % 10, toDisplayColor(gDigit1Color));
  gDisplay.renderDigit(1, mins % 10, toDisplayColor(gDigit2Color));
  gDisplay.renderDigit(2, (secs / 10) % 10, toDisplayColor(gSecondsColor));
  gDisplay.renderDigit(3, secs % 10, toDisplayColor(gSecondsColor));
  gDisplay.setColon(true, true, toColorScaled(gAccentColor, gColonBrightness));
  gDisplay.show();
}

static void renderColorDemo() {
  const uint8_t phase = gColorDemoStep % 6;
  RgbColor rainbow[6] = {
    {255, 0, 0},
    {255, 120, 0},
    {255, 255, 0},
    {0, 255, 0},
    {0, 120, 255},
    {180, 0, 255},
  };

  clearDisplay();
  gDisplay.renderDigit(0, 1, toDisplayColor(rainbow[(phase + 0) % 6]));
  gDisplay.renderDigit(1, 2, toDisplayColor(rainbow[(phase + 1) % 6]));
  gDisplay.renderDigit(2, 3, toDisplayColor(rainbow[(phase + 2) % 6]));
  gDisplay.renderDigit(3, 4, toDisplayColor(rainbow[(phase + 3) % 6]));
  gDisplay.setColon(true, true, toColorScaled(rainbow[(phase + 4) % 6], gColonBrightness));
  gDisplay.setDecimal(true, toDisplayColor(rainbow[(phase + 5) % 6]));
  gDisplay.show();
}

static void renderActiveMode() {
  switch (gMode) {
    case ClockMode::CLOCK:
      renderClock(false);
      break;
    case ClockMode::CLOCK_SECONDS:
      renderClock(true);
      break;
    case ClockMode::TIMER:
      renderTimer();
      break;
    case ClockMode::STOPWATCH:
      renderStopwatch();
      break;
    case ClockMode::COLOR_DEMO:
      renderColorDemo();
      break;
  }
}

static void updateTimer() {
  if (!gTimerRunning) {
    return;
  }

  const uint32_t now = millis();
  while (now - gTimerLastTickMs >= 1000UL) {
    gTimerLastTickMs += 1000UL;
    if (gTimerRemainingSeconds > 0) {
      gTimerRemainingSeconds--;
    }
    if (gTimerRemainingSeconds == 0) {
      gTimerRunning = false;
      replyOK("TIMER DONE");
      break;
    }
  }
}

static uint32_t getElapsedWhileRunning(uint32_t baseElapsed, uint32_t startMs) {
  if (!gStopwatchRunning) {
    return baseElapsed;
  }
  return baseElapsed + (millis() - startMs);
}

static void sendRtcRead() {
  if (!gRtcPresent) {
    Serial.println("RTC STATUS MISSING");
    return;
  }

  const DateTime now = gRtc.now();
  Serial.print("RTC ");
  Serial.print(now.year());
  Serial.print('-');
  if (now.month() < 10) Serial.print('0');
  Serial.print(now.month());
  Serial.print('-');
  if (now.day() < 10) Serial.print('0');
  Serial.print(now.day());
  Serial.print(' ');
  if (now.hour() < 10) Serial.print('0');
  Serial.print(now.hour());
  Serial.print(':');
  if (now.minute() < 10) Serial.print('0');
  Serial.print(now.minute());
  Serial.print(':');
  if (now.second() < 10) Serial.print('0');
  Serial.print(now.second());
  Serial.print(" LOST_POWER=");
  Serial.println(gRtcLostPower ? "1" : "0");
}

static bool parseIntToken(const String& text, int& value) {
  if (text.length() == 0) return false;
  for (uint16_t i = 0; i < text.length(); ++i) {
    if (!isDigit(text[i]) && !(i == 0 && text[i] == '-')) {
      return false;
    }
  }
  value = text.toInt();
  return true;
}

static bool parseRtcSet(const String& line, DateTime& out) {
  const String prefix = "RTC SET ";
  if (!line.startsWith(prefix)) {
    return false;
  }

  String payload = line.substring(prefix.length());
  payload.trim();

  int values[6] = {0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 6; ++i) {
    int split = payload.indexOf(' ');
    String token;
    if (split == -1) {
      token = payload;
      payload = "";
    } else {
      token = payload.substring(0, split);
      payload = payload.substring(split + 1);
      payload.trim();
    }
    if (!parseIntToken(token, values[i])) {
      return false;
    }
  }

  out = DateTime(values[0], values[1], values[2], values[3], values[4], values[5]);
  return true;
}

static bool startsWithCommand(const String& line, const char* command) {
  return line.startsWith(command);
}

static void replyOK(const String& text) {
  Serial.print("OK ");
  Serial.println(text);
}

static void replyERR(const String& text) {
  Serial.print("ERR ");
  Serial.println(text);
}

static void resetTimerToPreset() {
  gTimerRemainingSeconds = gTimerPresetSeconds;
}

static void processClockCommand(const String& rawLine) {
  String line = rawLine;
  line.trim();
  if (line.length() == 0) {
    return;
  }

  if (startsWithCommand(line, "MODE CLOCK_SECONDS")) {
    gMode = ClockMode::CLOCK_SECONDS;
    gSettingsDirty = true;
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("MODE CLOCK_SECONDS");
    return;
  }

  if (startsWithCommand(line, "MODE CLOCK")) {
    gMode = ClockMode::CLOCK;
    gSettingsDirty = true;
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("MODE CLOCK");
    return;
  }

  if (startsWithCommand(line, "MODE TIMER")) {
    gMode = ClockMode::TIMER;
    gSettingsDirty = true;
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("MODE TIMER");
    return;
  }

  if (startsWithCommand(line, "MODE STOPWATCH")) {
    gMode = ClockMode::STOPWATCH;
    gSettingsDirty = true;
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("MODE STOPWATCH");
    return;
  }

  if (startsWithCommand(line, "MODE COLOR_DEMO")) {
    gMode = ClockMode::COLOR_DEMO;
    gSettingsDirty = true;
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("MODE COLOR_DEMO");
    return;
  }

  if (line == "HOURMODE 24H") {
    gHourMode = HourMode::H24;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("HOURMODE 24H");
    return;
  }

  if (line == "HOURMODE 12H") {
    gHourMode = HourMode::H12;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("HOURMODE 12H");
    return;
  }

  if (line == "SECONDSMODE BLINK") {
    gSecondsMode = SecondsDisplayMode::BLINK;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("SECONDSMODE BLINK");
    return;
  }

  if (line == "SECONDSMODE ON") {
    gSecondsMode = SecondsDisplayMode::ON;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("SECONDSMODE ON");
    return;
  }

  if (line == "SECONDSMODE OFF") {
    gSecondsMode = SecondsDisplayMode::OFF;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("SECONDSMODE OFF");
    return;
  }

  if (line == "PRESET RAINBOW") {
    gDigit1Color = {255, 0, 0};
    gDigit2Color = {255, 120, 0};
    gDigit3Color = {0, 255, 0};
    gDigit4Color = {0, 120, 255};
    gAccentColor = {180, 0, 255};
    gSecondsColor = {0, 120, 255};
    gTimerColor = {180, 0, 255};
    gRainbowClockMode = true;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("PRESET RAINBOW");
    return;
  }

  if (line == "PRESET PARTY") {
    gDigit1Color = {255, 0, 255};
    gDigit2Color = {255, 0, 255};
    gDigit3Color = {255, 0, 255};
    gDigit4Color = {255, 0, 255};
    gAccentColor = {0, 255, 255};
    gSecondsColor = {255, 255, 0};
    gTimerColor = {255, 80, 80};
    gRainbowClockMode = false;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("PRESET PARTY");
    return;
  }
  if (startsWithCommand(line, "COLONBRIGHTNESS ")) {
    const int value = line.substring(String("COLONBRIGHTNESS ").length()).toInt();
    const uint8_t clipped = (uint8_t)constrain(value, 0, 255);
    gColonBrightness = clipped;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK(String("COLONBRIGHTNESS ") + clipped);
    return;
  }

  if (startsWithCommand(line, "BRIGHTNESS ")) {
    const int value = line.substring(String("BRIGHTNESS ").length()).toInt();
    const uint8_t clipped = (uint8_t)constrain(value, 1, 255);
    gBrightness = clipped;
    gPixels.setBrightness(255);
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK(String("BRIGHTNESS ") + clipped);
    return;
  }

  if (startsWithCommand(line, "COLOR ")) {
    String payload = line.substring(6);
    payload.trim();

    const int s1 = payload.indexOf(' ');
    if (s1 < 0) {
      replyERR("COLOR FORMAT");
      return;
    }

    const String target = payload.substring(0, s1);
    payload = payload.substring(s1 + 1);
    payload.trim();

    int vals[3] = {0, 0, 0};
    for (int i = 0; i < 3; ++i) {
      const int split = payload.indexOf(' ');
      String token;
      if (split == -1) {
        token = payload;
        payload = "";
      } else {
        token = payload.substring(0, split);
        payload = payload.substring(split + 1);
        payload.trim();
      }
      if (!parseIntToken(token, vals[i])) {
        replyERR("COLOR FORMAT");
        return;
      }
      vals[i] = constrain(vals[i], 0, 255);
    }

    RgbColor newColor = {(uint8_t)vals[0], (uint8_t)vals[1], (uint8_t)vals[2]};

    if (target == "CLOCK") {
      gDigit1Color = newColor;
      gDigit2Color = newColor;
      gDigit3Color = newColor;
      gDigit4Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "HOURS") {
      gDigit1Color = newColor;
      gDigit2Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "MINUTES") {
      gDigit3Color = newColor;
      gDigit4Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "DIGIT1") {
      gDigit1Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "DIGIT2") {
      gDigit2Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "DIGIT3") {
      gDigit3Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "DIGIT4") {
      gDigit4Color = newColor;
      gRainbowClockMode = false;
    } else if (target == "ACCENT") {
      gAccentColor = newColor;
    } else if (target == "SECONDS") {
      gSecondsColor = newColor;
    } else if (target == "TIMER") {
      gTimerColor = newColor;
    } else {
      replyERR("COLOR TARGET");
      return;
    }

    gSettingsDirty = true;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK(String("COLOR ") + target);
    return;
  }

  if (line == "RTC READ") {
    sendRtcRead();
    return;
  }

  if (startsWithCommand(line, "RTC SET ")) {
    if (!gRtcPresent) {
      replyERR("RTC MISSING");
      return;
    }

    DateTime dt;
    if (!parseRtcSet(line, dt)) {
      replyERR("RTC FORMAT");
      return;
    }

    gRtc.adjust(dt);
    gRtcLostPower = false;
    replyOK("RTC SET");
    sendRtcRead();
    return;
  }

  if (startsWithCommand(line, "TIMER SET ")) {
    const int value = line.substring(String("TIMER SET ").length()).toInt();
    gTimerPresetSeconds = (uint32_t)max(0, value);
    resetTimerToPreset();
    gTimerRunning = false;
    gSettingsDirty = true;
    if (!gCal.isCalibrationMode() && gMode == ClockMode::TIMER) renderActiveMode();
    replyOK(String("TIMER SET ") + gTimerPresetSeconds);
    return;
  }

  if (line == "TIMER START") {
    gMode = ClockMode::TIMER;
    gTimerRunning = true;
    gTimerLastTickMs = millis();
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("TIMER START");
    return;
  }

  if (line == "TIMER STOP") {
    gTimerRunning = false;
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("TIMER STOP");
    return;
  }

  if (line == "TIMER RESET") {
    gTimerRunning = false;
    resetTimerToPreset();
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("TIMER RESET");
    return;
  }

  if (line == "STOPWATCH START") {
    gMode = ClockMode::STOPWATCH;
    if (!gStopwatchRunning) {
      gStopwatchRunning = true;
      gStopwatchStartMs = millis();
    }
    gCal.setCalibrationMode(false);
    renderActiveMode();
    replyOK("STOPWATCH START");
    return;
  }

  if (line == "STOPWATCH STOP") {
    if (gStopwatchRunning) {
      gStopwatchAccumulatedMs += millis() - gStopwatchStartMs;
      gStopwatchRunning = false;
    }
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("STOPWATCH STOP");
    return;
  }

  if (line == "STOPWATCH RESET") {
    gStopwatchRunning = false;
    gStopwatchAccumulatedMs = 0;
    gStopwatchStartMs = millis();
    if (!gCal.isCalibrationMode()) renderActiveMode();
    replyOK("STOPWATCH RESET");
    return;
  }

  replyERR(String("UNKNOWN ") + line);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  gPixels.begin();
  gPixels.setBrightness(255);
  loadSettings();
  gButtons.begin();
  gMap.loadOrDefault();
  gMap.buildInverse();
  gCal.begin();

  gRtcPresent = gRtc.begin();
  if (gRtcPresent) {
    gRtcLostPower = gRtc.lostPower();
    if (gRtcLostPower) {
      gRtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      gRtcLostPower = false;
    }
  }

  resetTimerToPreset();
  gLastRenderMs = millis();
  renderActiveMode();
}

void loop() {
  gButtons.update();

  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (gSerialLine.length() > 0) {
        if (!gCal.handleCommand(gSerialLine)) {
          processClockCommand(gSerialLine);
        }
        gSerialLine = "";
      }
      continue;
    }
    if (gSerialLine.length() < Timing::kSerialLineMax) {
      gSerialLine += c;
    } else {
      gSerialLine = "";
      replyERR("LINE TOO LONG");
    }
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN1)) {
    gCal.setCalibrationMode(!gCal.isCalibrationMode());
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN2)) {
    if (gCal.isCalibrationMode()) {
      gCal.nextPhys(+1);
    } else {
      if (gMode == ClockMode::TIMER) {
        gTimerPresetSeconds += 60UL;
        resetTimerToPreset();
      } else {
        gMode = (ClockMode)(((uint8_t)gMode + 1U) % 5U);
      }
    }
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN3)) {
    if (gCal.isCalibrationMode()) {
      gCal.nextPhys(-1);
    } else {
      if (gMode == ClockMode::TIMER) {
        if (gTimerPresetSeconds >= 60UL) {
          gTimerPresetSeconds -= 60UL;
        } else {
          gTimerPresetSeconds = 0;
        }
        resetTimerToPreset();
      } else if (gMode == ClockMode::STOPWATCH) {
        if (gStopwatchRunning) {
          gStopwatchAccumulatedMs += millis() - gStopwatchStartMs;
          gStopwatchRunning = false;
        } else {
          gStopwatchRunning = true;
          gStopwatchStartMs = millis();
        }
      }
    }
  }

  const uint32_t now = millis();

  if (gSettingsDirty && (now - gLastSettingsSaveMs >= 750UL)) {
    saveSettings();
  }

  if (!gCal.isCalibrationMode()) {
    updateTimer();

    if (now - gLastRenderMs >= Timing::kClockRefreshMs) {
      gLastRenderMs = now;
      gColorDemoStep++;
      renderActiveMode();
    }
  }
}