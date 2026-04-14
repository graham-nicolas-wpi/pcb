#pragma once

#include <Arduino.h>

#include "../ui/Modes.h"
#include "../util/Color.h"

struct ClockSettings {
  uint8_t brightness;
  ClockMode mode;
  HourMode hourMode;
  SecondsDisplayMode secondsMode;
  TimerDisplayFormat timerDisplayFormat;
  uint8_t colonBrightness;
  bool rainbowClockMode;
  uint32_t timerPresetSeconds;
  RgbColor digit1Color;
  RgbColor digit2Color;
  RgbColor digit3Color;
  RgbColor digit4Color;
  RgbColor accentColor;
  RgbColor secondsColor;
  RgbColor timerColor;
  RgbColor timerHoursColor;
  RgbColor timerMinutesColor;
  RgbColor timerSecondsColor;
  RgbColor stopwatchHoursColor;
  RgbColor stopwatchMinutesColor;
  RgbColor stopwatchSecondsColor;
};

class SettingsStore {
 public:
  explicit SettingsStore(int eepromAddr = 128);

  void begin();
  void restoreDefaults();
  bool load();
  void save();
  void tick(uint32_t nowMs);
  void markDirty();

  bool isDirty() const;
  const ClockSettings& settings() const;
  ClockSettings& mutableSettings();

  void setMode(ClockMode mode);
  void setHourMode(HourMode mode);
  void setSecondsDisplayMode(SecondsDisplayMode mode);
  void setTimerDisplayFormat(TimerDisplayFormat format);
  void setBrightness(uint8_t value);
  void setColonBrightness(uint8_t value);
  void setTimerPresetSeconds(uint32_t seconds);

  static constexpr uint32_t kSettingsMagic = 0x434C4B31UL;
  static constexpr uint8_t kSettingsVersion = 3;
  static constexpr uint32_t kDeferredSaveMs = 750UL;

 private:
  struct PersistedSettings {
    uint32_t magic;
    uint8_t version;
    uint8_t brightness;
    uint8_t mode;
    uint8_t hourMode;
    uint8_t secondsMode;
    uint8_t timerDisplayFormat;
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
    RgbColor timerHoursColor;
    RgbColor timerMinutesColor;
    RgbColor timerSecondsColor;
    RgbColor stopwatchHoursColor;
    RgbColor stopwatchMinutesColor;
    RgbColor stopwatchSecondsColor;
    uint16_t checksum;
  };

  int eepromAddr_;
  ClockSettings settings_;
  bool dirty_;
  uint32_t lastSaveMs_;
};
