#include "AppStateMachine.h"

#include "../util/Color.h"

AppStateMachine::AppStateMachine()
    : mode_(ClockMode::CLOCK),
      lastRefreshMs_(0),
      colorDemoStep_(0),
      timerRemainingSeconds_(0),
      timerRemainingMs_(0),
      timerRunning_(false),
      timerStartMs_(0),
      timerStartRemainingMs_(0),
      timerDoneLatched_(false),
      stopwatchRunning_(false),
      stopwatchAccumulatedMs_(0),
      stopwatchStartMs_(0) {}

void AppStateMachine::begin(const ClockSettings& settings, uint32_t nowMs) {
  mode_ = settings.mode;
  lastRefreshMs_ = nowMs;
  colorDemoStep_ = 0;
  resetTimer(settings.timerPresetSeconds);
  timerStartMs_ = nowMs;
  timerStartRemainingMs_ = timerRemainingMs_;
  timerDoneLatched_ = false;
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  stopwatchStartMs_ = nowMs;
}

bool AppStateMachine::update(uint32_t nowMs) {
  bool renderRequested = false;

  if ((nowMs - lastRefreshMs_) >= kRefreshMs) {
    lastRefreshMs_ = nowMs;
    ++colorDemoStep_;
    renderRequested = true;
  }

  if (timerRunning_) {
    const uint32_t elapsedMs = nowMs - timerStartMs_;
    if (elapsedMs >= timerStartRemainingMs_) {
      timerRemainingMs_ = 0;
      timerRemainingSeconds_ = 0;
      timerRunning_ = false;
      timerDoneLatched_ = true;
      renderRequested = true;
    } else {
      timerRemainingMs_ = timerStartRemainingMs_ - elapsedMs;
      timerRemainingSeconds_ = (timerRemainingMs_ + 999UL) / 1000UL;
    }
  }

  return renderRequested;
}

bool AppStateMachine::consumeTimerDoneEvent() {
  if (!timerDoneLatched_) {
    return false;
  }
  timerDoneLatched_ = false;
  return true;
}

ClockMode AppStateMachine::mode() const {
  return mode_;
}

void AppStateMachine::setMode(ClockMode mode) {
  mode_ = mode;
}

void AppStateMachine::cycleModeForward() {
  const uint8_t next = (static_cast<uint8_t>(mode_) + 1U) % kClockModeCount;
  mode_ = static_cast<ClockMode>(next);
}

void AppStateMachine::startTimer(uint32_t nowMs, uint32_t presetSeconds) {
  mode_ = ClockMode::TIMER;
  if (timerRemainingMs_ == 0U) {
    timerRemainingMs_ = presetSeconds * 1000UL;
    timerRemainingSeconds_ = presetSeconds;
  }
  if (timerRemainingMs_ == 0U) {
    timerRunning_ = false;
    timerDoneLatched_ = true;
    return;
  }
  timerDoneLatched_ = false;
  timerRunning_ = true;
  timerStartMs_ = nowMs;
  timerStartRemainingMs_ = timerRemainingMs_;
}

void AppStateMachine::stopTimer(uint32_t nowMs) {
  if (timerRunning_) {
    const uint32_t elapsedMs = nowMs - timerStartMs_;
    if (elapsedMs >= timerStartRemainingMs_) {
      timerRemainingMs_ = 0;
      timerRemainingSeconds_ = 0;
    } else {
      timerRemainingMs_ = timerStartRemainingMs_ - elapsedMs;
      timerRemainingSeconds_ = (timerRemainingMs_ + 999UL) / 1000UL;
    }
  }
  timerRunning_ = false;
}

void AppStateMachine::resetTimer(uint32_t presetSeconds) {
  timerRunning_ = false;
  timerRemainingSeconds_ = presetSeconds;
  timerRemainingMs_ = presetSeconds * 1000UL;
  timerStartRemainingMs_ = timerRemainingMs_;
  timerDoneLatched_ = false;
}

void AppStateMachine::setTimerPreset(uint32_t presetSeconds) {
  resetTimer(presetSeconds);
}

uint32_t AppStateMachine::timerRemainingSeconds() const {
  return timerRemainingSeconds_;
}

uint32_t AppStateMachine::timerRemainingMs() const {
  return timerRemainingMs_;
}

bool AppStateMachine::timerRunning() const {
  return timerRunning_;
}

void AppStateMachine::startStopwatch(uint32_t nowMs) {
  mode_ = ClockMode::STOPWATCH;
  if (!stopwatchRunning_) {
    stopwatchRunning_ = true;
    stopwatchStartMs_ = nowMs;
  }
}

void AppStateMachine::stopStopwatch(uint32_t nowMs) {
  if (stopwatchRunning_) {
    stopwatchAccumulatedMs_ += nowMs - stopwatchStartMs_;
    stopwatchRunning_ = false;
  }
}

void AppStateMachine::toggleStopwatch(uint32_t nowMs) {
  if (stopwatchRunning_) {
    stopStopwatch(nowMs);
  } else {
    startStopwatch(nowMs);
  }
}

void AppStateMachine::resetStopwatch(uint32_t nowMs) {
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  stopwatchStartMs_ = nowMs;
}

uint32_t AppStateMachine::stopwatchElapsedMs(uint32_t nowMs) const {
  if (!stopwatchRunning_) {
    return stopwatchAccumulatedMs_;
  }
  return stopwatchAccumulatedMs_ + (nowMs - stopwatchStartMs_);
}

bool AppStateMachine::stopwatchRunning() const {
  return stopwatchRunning_;
}

void AppStateMachine::buildFrame(SegmentFrame& frame, const ClockSettings& settings,
                                 const DateTime& now, uint32_t nowMs) {
  FaceRenderState state = {
      frame,
      settings,
      {0, 0, 0, 0},
      false,
      false,
      false,
      false,
      false,
      colorDemoStep_,
      {settings.digit1Color, settings.digit2Color, settings.digit3Color, settings.digit4Color},
      settings.accentColor,
      settings.secondsColor};

  switch (mode_) {
    case ClockMode::CLOCK:
    case ClockMode::CLOCK_SECONDS: {
      uint8_t hours = now.hour();
      if (settings.hourMode == HourMode::H12) {
        hours = static_cast<uint8_t>(hours % 12U);
        if (hours == 0U) {
          hours = 12U;
        }
      }

      state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
      state.digits[1] = static_cast<uint8_t>(hours % 10U);
      state.digits[2] = static_cast<uint8_t>((now.minute() / 10U) % 10U);
      state.digits[3] = static_cast<uint8_t>(now.minute() % 10U);
      state.blankLeadingDigit = (settings.hourMode == HourMode::H12 && hours < 10U);
      switch (settings.secondsMode) {
        case SecondsDisplayMode::BLINK:
          state.colonTopOn = state.colonBottomOn = ((now.second() % 2U) == 0U);
          break;
        case SecondsDisplayMode::ON:
          state.colonTopOn = state.colonBottomOn = true;
          break;
        case SecondsDisplayMode::OFF:
          state.colonTopOn = state.colonBottomOn = false;
          break;
      }
      state.decimalOn = false;
      state.rainbowDigits = settings.rainbowClockMode;
      classicClockFace_.render(state);
      return;
    }

    case ClockMode::TIMER: {
      TimerDisplayFormat timerFormat = settings.timerDisplayFormat;
      if (timerFormat == TimerDisplayFormat::AUTO) {
        if (timerRemainingMs_ >= 3600000UL) {
          timerFormat = TimerDisplayFormat::HH_MM;
        } else if (timerRemainingMs_ >= 60000UL) {
          timerFormat = TimerDisplayFormat::MM_SS;
        } else {
          timerFormat = TimerDisplayFormat::SS_CS;
        }
      }

      if (timerFormat == TimerDisplayFormat::HH_MM) {
        const uint32_t totalMinutes = (timerRemainingMs_ + 59999UL) / 60000UL;
        const uint16_t hours = static_cast<uint16_t>((totalMinutes / 60UL) % 100UL);
        const uint8_t minutes = static_cast<uint8_t>(totalMinutes % 60UL);
        state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(hours % 10U);
        state.digits[2] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(minutes % 10U);
        state.digitColors[0] = settings.timerHoursColor;
        state.digitColors[1] = settings.timerHoursColor;
        state.digitColors[2] = settings.timerMinutesColor;
        state.digitColors[3] = settings.timerMinutesColor;
        state.colonTopOn = true;
        state.colonBottomOn = true;
        state.decimalOn = false;
        state.decimalColor = settings.timerMinutesColor;
      } else if (timerFormat == TimerDisplayFormat::MM_SS) {
        const uint32_t totalSeconds = (timerRemainingMs_ + 999UL) / 1000UL;
        const uint16_t minutes = static_cast<uint16_t>((totalSeconds / 60UL) % 100UL);
        const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
        state.digits[0] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(minutes % 10U);
        state.digits[2] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(seconds % 10U);
        state.digitColors[0] = settings.timerMinutesColor;
        state.digitColors[1] = settings.timerMinutesColor;
        state.digitColors[2] = settings.timerSecondsColor;
        state.digitColors[3] = settings.timerSecondsColor;
        state.colonTopOn = true;
        state.colonBottomOn = true;
        state.decimalOn = false;
        state.decimalColor = settings.timerSecondsColor;
      } else {
        const uint32_t totalTenths = timerRemainingMs_ / 100UL;
        const uint8_t seconds = static_cast<uint8_t>((totalTenths / 10UL) % 100UL);
        const uint8_t tenths = static_cast<uint8_t>(totalTenths % 10UL);
        state.digits[0] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(seconds % 10U);
        state.digits[2] = tenths;
        state.digits[3] = 255U;
        state.digitColors[0] = settings.timerSecondsColor;
        state.digitColors[1] = settings.timerSecondsColor;
        state.digitColors[2] = settings.timerSecondsColor;
        state.digitColors[3] = settings.timerSecondsColor;
        state.blankLeadingDigit = (seconds < 10U);
        state.colonTopOn = false;
        state.colonBottomOn = false;
        state.decimalOn = true;
        state.decimalColor = settings.timerSecondsColor;
      }
      timerFace_.render(state);
      return;
    }

    case ClockMode::STOPWATCH: {
      const uint32_t elapsedMs = stopwatchElapsedMs(nowMs);
      if (elapsedMs < 60000UL) {
        const uint32_t totalTenths = elapsedMs / 100UL;
        const uint8_t seconds = static_cast<uint8_t>((totalTenths / 10UL) % 100UL);
        const uint8_t tenths = static_cast<uint8_t>(totalTenths % 10UL);
        state.digits[0] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(seconds % 10U);
        state.digits[2] = tenths;
        state.digits[3] = 255U;
        state.digitColors[0] = settings.stopwatchSecondsColor;
        state.digitColors[1] = settings.stopwatchSecondsColor;
        state.digitColors[2] = settings.stopwatchSecondsColor;
        state.digitColors[3] = settings.stopwatchSecondsColor;
        state.blankLeadingDigit = (seconds < 10U);
        state.colonTopOn = false;
        state.colonBottomOn = false;
        state.decimalOn = true;
        state.decimalColor = settings.stopwatchSecondsColor;
      } else if (elapsedMs < 3600000UL) {
        const uint32_t totalSeconds = elapsedMs / 1000UL;
        const uint16_t minutes = static_cast<uint16_t>((totalSeconds / 60UL) % 100UL);
        const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
        state.digits[0] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(minutes % 10U);
        state.digits[2] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(seconds % 10U);
        state.digitColors[0] = settings.stopwatchMinutesColor;
        state.digitColors[1] = settings.stopwatchMinutesColor;
        state.digitColors[2] = settings.stopwatchSecondsColor;
        state.digitColors[3] = settings.stopwatchSecondsColor;
        state.colonTopOn = true;
        state.colonBottomOn = true;
        state.decimalOn = false;
        state.decimalColor = settings.stopwatchSecondsColor;
      } else {
        const uint32_t totalMinutes = elapsedMs / 60000UL;
        const uint16_t hours = static_cast<uint16_t>((totalMinutes / 60UL) % 100UL);
        const uint8_t minutes = static_cast<uint8_t>(totalMinutes % 60UL);
        state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(hours % 10U);
        state.digits[2] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(minutes % 10U);
        state.digitColors[0] = settings.stopwatchHoursColor;
        state.digitColors[1] = settings.stopwatchHoursColor;
        state.digitColors[2] = settings.stopwatchMinutesColor;
        state.digitColors[3] = settings.stopwatchMinutesColor;
        state.colonTopOn = true;
        state.colonBottomOn = true;
        state.decimalOn = false;
        state.decimalColor = settings.stopwatchMinutesColor;
      }
      timerFace_.render(state);
      return;
    }

    case ClockMode::COLOR_DEMO: {
      const uint8_t phase = static_cast<uint8_t>(colorDemoStep_ % 6U);
      const RgbColor rainbow[6] = {
          makeRgbColor(255, 0, 0),   makeRgbColor(255, 120, 0),
          makeRgbColor(255, 255, 0), makeRgbColor(0, 255, 0),
          makeRgbColor(0, 120, 255), makeRgbColor(180, 0, 255),
      };
      state.digits[0] = 1;
      state.digits[1] = 2;
      state.digits[2] = 3;
      state.digits[3] = 4;
      state.digitColors[0] = rainbow[(phase + 0U) % 6U];
      state.digitColors[1] = rainbow[(phase + 1U) % 6U];
      state.digitColors[2] = rainbow[(phase + 2U) % 6U];
      state.digitColors[3] = rainbow[(phase + 3U) % 6U];
      state.colonColor = rainbow[(phase + 4U) % 6U];
      state.decimalColor = rainbow[(phase + 5U) % 6U];
      state.colonTopOn = true;
      state.colonBottomOn = true;
      state.decimalOn = true;
      timerFace_.render(state);
      return;
    }
  }
}
