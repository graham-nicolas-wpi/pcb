#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <RTClib.h>

#include "../display/LedMap.h"
#include "../display/LogicalDisplay.h"
#include "../hal/Buttons.h"
#include "../hal/PixelDriver_NeoPixel.h"
#include "../model/ClockConfig.h"
#include "../storage/SettingsStore.h"

class ClockRuntime {
 public:
  ClockRuntime(PixelDriver_NeoPixel& pixels, Buttons& buttons, LedMap& map,
               LogicalDisplay& display, SettingsStore& store);

  void begin();
  void update(uint32_t nowMs);

  void writeInfoJson(JsonObject root) const;
  void writeStatusJson(JsonObject root, uint32_t nowMs) const;
  void writeRtcJson(JsonObject root, uint32_t nowMs) const;

  void reloadFromConfig();
  bool saveConfigNow(String* error = nullptr);
  bool loadConfigNow(String* error = nullptr);

  void setMode(ClockMode mode);
  void setHourMode(HourMode mode);
  void setSecondsMode(SecondsMode mode);
  void setTimerFormat(TimerDisplayFormat format);
  void setBrightness(uint8_t brightness);
  void setColonBrightness(uint8_t brightness);
  void setEffect(EffectMode effect);
  bool setActivePreset(const String& id);
  bool setActiveFace(const String& id);
  bool setColorTarget(const String& target, const RgbColor& color);

  void setTimerPresetSeconds(uint32_t seconds);
  void startTimer(uint32_t nowMs);
  void stopTimer(uint32_t nowMs);
  void resetTimer();

  void startStopwatch(uint32_t nowMs);
  void stopStopwatch(uint32_t nowMs);
  void resetStopwatch(uint32_t nowMs);

  void setCalibrationMode(bool enabled);
  void nextCalibration(int8_t delta);
  bool setCalibrationCursor(uint8_t physical);
  bool assignLogical(uint8_t logical);
  void unassignCurrent();
  bool validateMapping() const;
  void testSegments(uint16_t msPerSegment = 180);
  void testDigits(uint16_t msPerFrame = 300);
  void testAll();

  bool setRtc(const DateTime& value);
  bool rtcPresent() const;
  bool rtcLostPower() const;

  uint32_t timerRemainingSeconds() const;
  bool timerRunning() const;
  bool stopwatchRunning() const;
  uint8_t calibrationCursor() const;
  bool calibrationMode() const;

 private:
  struct RenderState {
    uint8_t digits[4];
    RgbColor digitColors[4];
    bool blankLeadingDigit;
    bool colonTopOn;
    bool colonBottomOn;
    bool decimalOn;
    RgbColor colonColor;
    RgbColor decimalColor;
  };

  const ClockConfig& config() const;
  ClockConfig& mutableConfig();
  const PresetDefinition* activePreset() const;
  const FaceDefinition* activeFaceForMode() const;
  const WidgetDefinition* widgetById(const String& id) const;

  void handleButtons(uint32_t nowMs);
  void applyMapping();
  void markDirty(bool persist = true);
  void updateTimerState(uint32_t nowMs);
  void renderIfNeeded(uint32_t nowMs);
  void renderCalibration();
  void renderRegular(uint32_t nowMs);
  void buildRenderState(RenderState& state, uint32_t nowMs) const;
  void applyEffect(RenderState& state, EffectMode effect, uint32_t nowMs) const;
  void applyWidgets(RenderState& state, const FaceDefinition* face, uint32_t nowMs) const;
  DateTime fallbackNow(uint32_t nowMs) const;
  DateTime currentTime(uint32_t nowMs) const;
  uint32_t stopwatchElapsedMs(uint32_t nowMs) const;
  uint8_t pulseValue(uint32_t nowMs, uint8_t speed) const;
  uint32_t toColor(const RgbColor& color) const;
  void syncFaceToMode();

  PixelDriver_NeoPixel& pixels_;
  Buttons& buttons_;
  LedMap& map_;
  LogicalDisplay& display_;
  SettingsStore& store_;
  RTC_DS3231 rtc_;
  bool rtcPresent_ = false;
  bool rtcLostPower_ = false;
  bool renderDirty_ = true;
  bool persistPending_ = false;
  uint32_t lastSaveRequestMs_ = 0;
  uint32_t lastRenderMs_ = 0;
  uint32_t timerRemainingMs_ = 0;
  bool timerRunning_ = false;
  uint32_t timerStartedMs_ = 0;
  uint32_t timerStartedRemainingMs_ = 0;
  bool stopwatchRunning_ = false;
  uint32_t stopwatchStartedMs_ = 0;
  uint32_t stopwatchAccumulatedMs_ = 0;
};
