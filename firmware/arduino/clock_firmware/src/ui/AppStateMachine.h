#pragma once

#include <Arduino.h>
#include <RTClib.h>

#include "../display/LogicalDisplay.h"
#include "../faces/Face_ClassicClock.h"
#include "../faces/Face_Timer.h"
#include "../settings/SettingsStore.h"
#include "Modes.h"

class AppStateMachine {
 public:
  AppStateMachine();

  void begin(const ClockSettings& settings, uint32_t nowMs);
  bool update(uint32_t nowMs);
  bool consumeTimerDoneEvent();

  ClockMode mode() const;
  void setMode(ClockMode mode);
  void cycleModeForward();

  void startTimer(uint32_t nowMs, uint32_t presetSeconds);
  void stopTimer(uint32_t nowMs);
  void resetTimer(uint32_t presetSeconds);
  void setTimerPreset(uint32_t presetSeconds);
  uint32_t timerRemainingSeconds() const;
  uint32_t timerRemainingMs() const;
  bool timerRunning() const;

  void startStopwatch(uint32_t nowMs);
  void stopStopwatch(uint32_t nowMs);
  void toggleStopwatch(uint32_t nowMs);
  void resetStopwatch(uint32_t nowMs);
  uint32_t stopwatchElapsedMs(uint32_t nowMs) const;
  bool stopwatchRunning() const;

  void render(LogicalDisplay& display, PixelDriver_NeoPixel& pixels,
              const ClockSettings& settings, const DateTime& now, uint32_t nowMs);

 private:
  static constexpr uint32_t kRefreshMs = 100UL;

  ClockMode mode_;
  uint32_t lastRefreshMs_;
  uint32_t colorDemoStep_;

  uint32_t timerRemainingSeconds_;
  uint32_t timerRemainingMs_;
  bool timerRunning_;
  uint32_t timerStartMs_;
  uint32_t timerStartRemainingMs_;
  bool timerDoneLatched_;

  bool stopwatchRunning_;
  uint32_t stopwatchAccumulatedMs_;
  uint32_t stopwatchStartMs_;

  Face_ClassicClock classicClockFace_;
  Face_Timer timerFace_;
};
