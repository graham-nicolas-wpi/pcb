#pragma once

#include <Arduino.h>

enum class ClockMode : uint8_t {
  CLOCK = 0,
  CLOCK_SECONDS,
  TIMER,
  STOPWATCH,
  COLOR_DEMO,
};

constexpr uint8_t kClockModeCount = static_cast<uint8_t>(ClockMode::COLOR_DEMO) + 1U;

enum class HourMode : uint8_t {
  H24 = 0,
  H12,
};

enum class SecondsDisplayMode : uint8_t {
  BLINK = 0,
  ON,
  OFF,
};

enum class TimerDisplayFormat : uint8_t {
  AUTO = 0,
  HH_MM,
  MM_SS,
  SS_CS,
};

inline ClockMode clampClockModeValue(uint8_t rawValue) {
  if (rawValue > static_cast<uint8_t>(ClockMode::COLOR_DEMO)) {
    return ClockMode::CLOCK;
  }
  return static_cast<ClockMode>(rawValue);
}

inline HourMode clampHourModeValue(uint8_t rawValue) {
  if (rawValue > static_cast<uint8_t>(HourMode::H12)) {
    return HourMode::H24;
  }
  return static_cast<HourMode>(rawValue);
}

inline SecondsDisplayMode clampSecondsDisplayModeValue(uint8_t rawValue) {
  if (rawValue > static_cast<uint8_t>(SecondsDisplayMode::OFF)) {
    return SecondsDisplayMode::BLINK;
  }
  return static_cast<SecondsDisplayMode>(rawValue);
}

inline const char* clockModeName(ClockMode mode) {
  switch (mode) {
    case ClockMode::CLOCK:
      return "CLOCK";
    case ClockMode::CLOCK_SECONDS:
      return "CLOCK_SECONDS";
    case ClockMode::TIMER:
      return "TIMER";
    case ClockMode::STOPWATCH:
      return "STOPWATCH";
    case ClockMode::COLOR_DEMO:
      return "COLOR_DEMO";
  }
  return "UNKNOWN";
}

inline const char* hourModeName(HourMode mode) {
  switch (mode) {
    case HourMode::H24:
      return "24H";
    case HourMode::H12:
      return "12H";
  }
  return "UNKNOWN";
}

inline const char* secondsModeName(SecondsDisplayMode mode) {
  switch (mode) {
    case SecondsDisplayMode::BLINK:
      return "BLINK";
    case SecondsDisplayMode::ON:
      return "ON";
    case SecondsDisplayMode::OFF:
      return "OFF";
  }
  return "UNKNOWN";
}

inline TimerDisplayFormat clampTimerDisplayFormatValue(uint8_t rawValue) {
  if (rawValue > static_cast<uint8_t>(TimerDisplayFormat::SS_CS)) {
    return TimerDisplayFormat::AUTO;
  }
  return static_cast<TimerDisplayFormat>(rawValue);
}

inline const char* timerDisplayFormatName(TimerDisplayFormat format) {
  switch (format) {
    case TimerDisplayFormat::AUTO:
      return "AUTO";
    case TimerDisplayFormat::HH_MM:
      return "HHMM";
    case TimerDisplayFormat::MM_SS:
      return "MMSS";
    case TimerDisplayFormat::SS_CS:
      return "SSCS";
  }
  return "UNKNOWN";
}
