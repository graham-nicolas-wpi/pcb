#pragma once

#include <ArduinoJson.h>
#include <RTClib.h>

#include "../host/LeonardoClient.h"
#include "../input/RotaryKnob.h"
#include "../render/FrameCompiler.h"
#include "../storage/SettingsStore.h"

class HostRuntime {
 public:
  HostRuntime(SettingsStore& store, LeonardoClient& renderer);

  void begin();
  void update(uint32_t nowMs);

  void writeInfoJson(JsonObject root) const;
  void writeStatusJson(JsonObject root, uint32_t nowMs) const;
  void writeRtcJson(JsonObject root, uint32_t nowMs) const;
  void writeRendererJson(JsonObject root, uint32_t nowMs) const;

  // Read-only accessors for the clockWinder TCP bridge.
  bool timerRunning() const { return timerRunning_; }
  bool stopwatchRunning() const { return stopwatchRunning_; }
  uint32_t timerRemainingSeconds() const { return (timerRemainingMs_ + 999UL) / 1000UL; }
  bool rendererOnline(uint32_t nowMs) const { return renderer_.isOnline(nowMs); }
  const LeonardoClient& renderer() const { return renderer_; }
  DateTime now(uint32_t nowMs) const { return currentTime(nowMs); }
  const char* timeSource() const { return timeSourceName(); }

  void reloadFromConfig();
  bool saveConfigNow(String* error = nullptr);
  bool loadConfigNow(String* error = nullptr);

  void setMode(ClockMode mode);
  void setHourMode(HourMode mode);
  void setSecondsMode(SecondsMode mode);
  void setTimerFormat(TimerDisplayFormat format);
  void setBrightness(uint8_t brightness);
  void setColonBrightness(uint8_t brightness);
  void setNightRed(bool enabled);
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

  bool setRtc(const DateTime& value);
  void requestRendererRefresh();
  void controlRendererHost();
  void controlRendererLocal();
  void runRendererTestSegments();
  void runRendererTestDigits();
  void runRendererTestAll();

  String exportRendererMappingJson() const;
  String exportRendererMapHeader() const;

 private:
  enum class TimeSource : uint8_t {
    FALLBACK = 0,
    RENDERER_RTC,
    NTP,
  };

  const ClockConfig& config() const;
  ClockConfig& mutableConfig();
  void markDirty(bool persist = true);
  void syncFaceToMode();
  void maintainRendererLink(uint32_t nowMs);
  void handleRendererEvents(uint32_t nowMs);
  void updateTimeSource(uint32_t nowMs);
  void updateTimerState(uint32_t nowMs);
  void renderIfNeeded(uint32_t nowMs);
  void handleKnob(uint32_t nowMs);
  void knobBrightness(int16_t detents, uint32_t nowMs);
  void knobTimerLength(int16_t detents);
  DateTime fallbackNow(uint32_t nowMs) const;
  DateTime currentTime(uint32_t nowMs) const;
  const char* timeSourceName() const;
  uint32_t stopwatchElapsedMs(uint32_t nowMs) const;

  SettingsStore& store_;
  LeonardoClient& renderer_;
  FrameCompiler compiler_;
  RotaryKnob knob_;
  uint32_t lastKnobTurnMs_ = 0;
  bool renderDirty_ = true;
  bool persistPending_ = false;
  uint32_t lastSaveRequestMs_ = 0;
  uint32_t lastRenderMs_ = 0;
  uint32_t lastHelloRequestMs_ = 0;
  uint32_t lastStatusPollMs_ = 0;
  uint32_t lastRtcPollMs_ = 0;
  uint32_t lastMapPollMs_ = 0;
  uint32_t lastControlRefreshMs_ = 0;
  uint32_t lastNtpPollMs_ = 0;
  uint32_t lastRtcSyncMs_ = 0;
  bool ntpConfigured_ = false;
  bool ntpValid_ = false;
  TimeSource timeSource_ = TimeSource::FALLBACK;
  uint32_t timerRemainingMs_ = 0;
  bool timerRunning_ = false;
  uint32_t timerStartedMs_ = 0;
  uint32_t timerStartedRemainingMs_ = 0;
  bool stopwatchRunning_ = false;
  uint32_t stopwatchStartedMs_ = 0;
  uint32_t stopwatchAccumulatedMs_ = 0;
};
