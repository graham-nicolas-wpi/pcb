#include "SettingsStore.h"

#include <EEPROM.h>

#include "../util/Checksums.h"

SettingsStore::SettingsStore(int eepromAddr)
    : eepromAddr_(eepromAddr), dirty_(false), lastSaveMs_(0) {
  restoreDefaults();
}

void SettingsStore::begin() {
  restoreDefaults();
  load();
  dirty_ = false;
  lastSaveMs_ = millis();
}

void SettingsStore::restoreDefaults() {
  settings_.brightness = 64;
  settings_.mode = ClockMode::CLOCK;
  settings_.hourMode = HourMode::H24;
  settings_.secondsMode = SecondsDisplayMode::BLINK;
  settings_.timerDisplayFormat = TimerDisplayFormat::AUTO;
  settings_.colonBrightness = 255;
  settings_.rainbowClockMode = false;
  settings_.timerPresetSeconds = 5UL * 60UL;
  settings_.digit1Color = makeRgbColor(255, 120, 40);
  settings_.digit2Color = makeRgbColor(255, 120, 40);
  settings_.digit3Color = makeRgbColor(255, 120, 40);
  settings_.digit4Color = makeRgbColor(255, 120, 40);
  settings_.accentColor = makeRgbColor(0, 120, 255);
  settings_.secondsColor = makeRgbColor(255, 255, 255);
  settings_.timerColor = makeRgbColor(80, 255, 120);
  settings_.timerHoursColor = settings_.timerColor;
  settings_.timerMinutesColor = settings_.timerColor;
  settings_.timerSecondsColor = settings_.timerColor;
  settings_.stopwatchHoursColor = settings_.digit1Color;
  settings_.stopwatchMinutesColor = settings_.digit2Color;
  settings_.stopwatchSecondsColor = settings_.secondsColor;
}

bool SettingsStore::load() {
  PersistedSettings persisted;
  EEPROM.get(eepromAddr_, persisted);

  if (persisted.magic != kSettingsMagic) {
    return false;
  }
  if (persisted.version != kSettingsVersion) {
    return false;
  }

  const uint16_t expected =
      checksum16(reinterpret_cast<const uint8_t*>(&persisted),
                 sizeof(PersistedSettings) - sizeof(uint16_t));
  if (persisted.checksum != expected) {
    return false;
  }

  settings_.brightness =
      static_cast<uint8_t>(constrain(static_cast<int>(persisted.brightness), 1, 255));
  settings_.mode = clampClockModeValue(persisted.mode);
  settings_.hourMode = clampHourModeValue(persisted.hourMode);
  settings_.secondsMode = clampSecondsDisplayModeValue(persisted.secondsMode);
  settings_.timerDisplayFormat =
      clampTimerDisplayFormatValue(persisted.timerDisplayFormat);
  settings_.colonBrightness = persisted.colonBrightness;
  settings_.rainbowClockMode = persisted.rainbowClockMode != 0U;
  settings_.timerPresetSeconds = persisted.timerPresetSeconds;
  settings_.digit1Color = persisted.digit1Color;
  settings_.digit2Color = persisted.digit2Color;
  settings_.digit3Color = persisted.digit3Color;
  settings_.digit4Color = persisted.digit4Color;
  settings_.accentColor = persisted.accentColor;
  settings_.secondsColor = persisted.secondsColor;
  settings_.timerColor = persisted.timerColor;
  settings_.timerHoursColor = persisted.timerHoursColor;
  settings_.timerMinutesColor = persisted.timerMinutesColor;
  settings_.timerSecondsColor = persisted.timerSecondsColor;
  settings_.stopwatchHoursColor = persisted.stopwatchHoursColor;
  settings_.stopwatchMinutesColor = persisted.stopwatchMinutesColor;
  settings_.stopwatchSecondsColor = persisted.stopwatchSecondsColor;
  return true;
}

void SettingsStore::save() {
  PersistedSettings persisted;
  persisted.magic = kSettingsMagic;
  persisted.version = kSettingsVersion;
  persisted.brightness = settings_.brightness;
  persisted.mode = static_cast<uint8_t>(settings_.mode);
  persisted.hourMode = static_cast<uint8_t>(settings_.hourMode);
  persisted.secondsMode = static_cast<uint8_t>(settings_.secondsMode);
  persisted.timerDisplayFormat = static_cast<uint8_t>(settings_.timerDisplayFormat);
  persisted.colonBrightness = settings_.colonBrightness;
  persisted.rainbowClockMode = settings_.rainbowClockMode ? 1U : 0U;
  persisted.timerPresetSeconds = settings_.timerPresetSeconds;
  persisted.digit1Color = settings_.digit1Color;
  persisted.digit2Color = settings_.digit2Color;
  persisted.digit3Color = settings_.digit3Color;
  persisted.digit4Color = settings_.digit4Color;
  persisted.accentColor = settings_.accentColor;
  persisted.secondsColor = settings_.secondsColor;
  persisted.timerColor = settings_.timerColor;
  persisted.timerHoursColor = settings_.timerHoursColor;
  persisted.timerMinutesColor = settings_.timerMinutesColor;
  persisted.timerSecondsColor = settings_.timerSecondsColor;
  persisted.stopwatchHoursColor = settings_.stopwatchHoursColor;
  persisted.stopwatchMinutesColor = settings_.stopwatchMinutesColor;
  persisted.stopwatchSecondsColor = settings_.stopwatchSecondsColor;
  persisted.checksum = 0;
  persisted.checksum =
      checksum16(reinterpret_cast<const uint8_t*>(&persisted),
                 sizeof(PersistedSettings) - sizeof(uint16_t));

  EEPROM.put(eepromAddr_, persisted);
  dirty_ = false;
  lastSaveMs_ = millis();
}

void SettingsStore::tick(uint32_t nowMs) {
  if (dirty_ && (nowMs - lastSaveMs_ >= kDeferredSaveMs)) {
    save();
  }
}

void SettingsStore::markDirty() {
  dirty_ = true;
}

bool SettingsStore::isDirty() const {
  return dirty_;
}

const ClockSettings& SettingsStore::settings() const {
  return settings_;
}

ClockSettings& SettingsStore::mutableSettings() {
  return settings_;
}

void SettingsStore::setMode(ClockMode mode) {
  settings_.mode = mode;
  markDirty();
}

void SettingsStore::setHourMode(HourMode mode) {
  settings_.hourMode = mode;
  markDirty();
}

void SettingsStore::setSecondsDisplayMode(SecondsDisplayMode mode) {
  settings_.secondsMode = mode;
  markDirty();
}

void SettingsStore::setTimerDisplayFormat(TimerDisplayFormat format) {
  settings_.timerDisplayFormat = format;
  markDirty();
}

void SettingsStore::setBrightness(uint8_t value) {
  settings_.brightness = static_cast<uint8_t>(constrain(static_cast<int>(value), 1, 255));
  markDirty();
}

void SettingsStore::setColonBrightness(uint8_t value) {
  settings_.colonBrightness = value;
  markDirty();
}

void SettingsStore::setTimerPresetSeconds(uint32_t seconds) {
  settings_.timerPresetSeconds = seconds;
  markDirty();
}
