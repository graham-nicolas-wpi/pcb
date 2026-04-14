#pragma once

#include <Arduino.h>

#include "../../include/project_config.h"
#include "../display/LogicalIds.h"
#include "../util/Color.h"

enum class ClockMode : uint8_t {
  CLOCK = 0,
  CLOCK_SECONDS,
  TIMER,
  STOPWATCH,
  COLOR_DEMO
};

enum class HourMode : uint8_t { H24 = 0, H12 };
enum class SecondsMode : uint8_t { BLINK = 0, ON, OFF };
enum class TimerDisplayFormat : uint8_t { AUTO = 0, HHMM, MMSS, SSCS };
enum class EffectMode : uint8_t { NONE = 0, RAINBOW, PULSE, PARTY };
enum class WidgetType : uint8_t { PULSE_COLON = 0, RAINBOW_DIGITS, TIMER_WARNING, SECONDS_SPARK, ACCENT_SWEEP };

struct ColorPalette {
  RgbColor digit1;
  RgbColor digit2;
  RgbColor digit3;
  RgbColor digit4;
  RgbColor accent;
  RgbColor seconds;
  RgbColor timerHours;
  RgbColor timerMinutes;
  RgbColor timerSeconds;
  RgbColor stopwatchHours;
  RgbColor stopwatchMinutes;
  RgbColor stopwatchSeconds;
};

// Legacy import compatibility only. The active ESP32 host runtime no longer owns board LEDs,
// buttons, or RTC wiring directly.
struct HardwareSettings {
  uint8_t pixelCount = ProjectConfig::kDefaultPixelCount;
  uint8_t pixelPin = ProjectConfig::kDefaultPixelPin;
  uint8_t buttonPins[3] = {
      ProjectConfig::kDefaultButton1Pin,
      ProjectConfig::kDefaultButton2Pin,
      ProjectConfig::kDefaultButton3Pin,
  };
  uint8_t rtcSda = ProjectConfig::kDefaultRtcSdaPin;
  uint8_t rtcScl = ProjectConfig::kDefaultRtcSclPin;
  String colorOrder = "GRB";
};

struct NetworkSettings {
  String apSsid = "NeoPixelClock";
  String apPassword = "clockclock";
  bool staEnabled = false;
  String staSsid;
  String staPassword;
  String hostname = "neopixel-clock";
};

struct RendererLinkSettings {
  uint32_t baud = ProjectConfig::kDefaultRendererBaud;
  uint8_t txPin = ProjectConfig::kDefaultRendererTxPin;
  uint8_t rxPin = ProjectConfig::kDefaultRendererRxPin;
  bool buttonEvents = true;
};

struct TimeSettings {
  bool ntpEnabled = true;
  String timezone = "EST5EDT,M3.2.0/2,M11.1.0/2";
  String ntpPrimary = "pool.ntp.org";
  String ntpSecondary = "time.nist.gov";
  uint32_t rtcSyncIntervalSeconds = 3600UL;
};

struct RuntimeSettings {
  ClockMode mode = ClockMode::CLOCK;
  HourMode hourMode = HourMode::H24;
  SecondsMode secondsMode = SecondsMode::BLINK;
  TimerDisplayFormat timerFormat = TimerDisplayFormat::AUTO;
  EffectMode effect = EffectMode::NONE;
  uint8_t brightness = 96;
  uint8_t colonBrightness = 160;
  bool rainbowEnabled = false;
  bool calibrationMode = false;
  uint8_t calibrationCursor = 0;
  uint32_t timerPresetSeconds = 300UL;
  String activePresetId = "warm-classic";
  String activeFaceId = "classic-clock";
};

struct PresetDefinition {
  String id;
  String name;
  ColorPalette palette;
  EffectMode effect = EffectMode::NONE;
  bool rainbowClock = false;
};

struct WidgetDefinition {
  String id;
  String name;
  WidgetType type = WidgetType::PULSE_COLON;
  bool enabled = true;
  RgbColor primary = makeRgbColor(255, 255, 255);
  RgbColor secondary = makeRgbColor(0, 120, 255);
  uint16_t thresholdSeconds = 30;
  uint8_t speed = 128;
};

struct FaceDefinition {
  String id;
  String name;
  ClockMode baseMode = ClockMode::CLOCK;
  String presetId;
  EffectMode effect = EffectMode::NONE;
  bool showSeconds = false;
  bool showColon = true;
  bool showDecimal = false;
  bool autoTimerFormat = true;
  String widgetIds[ProjectConfig::kMaxFaceWidgets];
  uint8_t widgetCount = 0;
};

// Legacy import compatibility only. Leonardo calibration remains the authoritative map writer.
struct MappingSettings {
  uint8_t physToLogical[kNumPixels];
};

struct ClockConfig {
  HardwareSettings hardware;
  NetworkSettings network;
  RendererLinkSettings rendererLink;
  TimeSettings time;
  RuntimeSettings runtime;
  MappingSettings mapping;
  PresetDefinition presets[ProjectConfig::kMaxPresets];
  size_t presetCount = 0;
  WidgetDefinition widgets[ProjectConfig::kMaxWidgets];
  size_t widgetCount = 0;
  FaceDefinition faces[ProjectConfig::kMaxFaces];
  size_t faceCount = 0;
};

inline const char* clockModeName(ClockMode mode) {
  switch (mode) {
    case ClockMode::CLOCK: return "CLOCK";
    case ClockMode::CLOCK_SECONDS: return "CLOCK_SECONDS";
    case ClockMode::TIMER: return "TIMER";
    case ClockMode::STOPWATCH: return "STOPWATCH";
    case ClockMode::COLOR_DEMO: return "COLOR_DEMO";
    default: return "CLOCK";
  }
}

inline ClockMode clockModeFromString(const String& value) {
  if (value == "CLOCK_SECONDS") return ClockMode::CLOCK_SECONDS;
  if (value == "TIMER") return ClockMode::TIMER;
  if (value == "STOPWATCH") return ClockMode::STOPWATCH;
  if (value == "COLOR_DEMO") return ClockMode::COLOR_DEMO;
  return ClockMode::CLOCK;
}

inline const char* hourModeName(HourMode mode) {
  return mode == HourMode::H12 ? "12H" : "24H";
}

inline HourMode hourModeFromString(const String& value) {
  return value == "12H" ? HourMode::H12 : HourMode::H24;
}

inline const char* secondsModeName(SecondsMode mode) {
  switch (mode) {
    case SecondsMode::ON: return "ON";
    case SecondsMode::OFF: return "OFF";
    default: return "BLINK";
  }
}

inline SecondsMode secondsModeFromString(const String& value) {
  if (value == "ON") return SecondsMode::ON;
  if (value == "OFF") return SecondsMode::OFF;
  return SecondsMode::BLINK;
}

inline const char* timerFormatName(TimerDisplayFormat mode) {
  switch (mode) {
    case TimerDisplayFormat::HHMM: return "HHMM";
    case TimerDisplayFormat::MMSS: return "MMSS";
    case TimerDisplayFormat::SSCS: return "SSCS";
    default: return "AUTO";
  }
}

inline TimerDisplayFormat timerFormatFromString(const String& value) {
  if (value == "HHMM") return TimerDisplayFormat::HHMM;
  if (value == "MMSS") return TimerDisplayFormat::MMSS;
  if (value == "SSCS") return TimerDisplayFormat::SSCS;
  return TimerDisplayFormat::AUTO;
}

inline const char* effectModeName(EffectMode mode) {
  switch (mode) {
    case EffectMode::RAINBOW: return "RAINBOW";
    case EffectMode::PULSE: return "PULSE";
    case EffectMode::PARTY: return "PARTY";
    default: return "NONE";
  }
}

inline EffectMode effectModeFromString(const String& value) {
  if (value == "RAINBOW") return EffectMode::RAINBOW;
  if (value == "PULSE") return EffectMode::PULSE;
  if (value == "PARTY") return EffectMode::PARTY;
  return EffectMode::NONE;
}

inline const char* widgetTypeName(WidgetType type) {
  switch (type) {
    case WidgetType::RAINBOW_DIGITS: return "RAINBOW_DIGITS";
    case WidgetType::TIMER_WARNING: return "TIMER_WARNING";
    case WidgetType::SECONDS_SPARK: return "SECONDS_SPARK";
    case WidgetType::ACCENT_SWEEP: return "ACCENT_SWEEP";
    default: return "PULSE_COLON";
  }
}

inline WidgetType widgetTypeFromString(const String& value) {
  if (value == "RAINBOW_DIGITS") return WidgetType::RAINBOW_DIGITS;
  if (value == "TIMER_WARNING") return WidgetType::TIMER_WARNING;
  if (value == "SECONDS_SPARK") return WidgetType::SECONDS_SPARK;
  if (value == "ACCENT_SWEEP") return WidgetType::ACCENT_SWEEP;
  return WidgetType::PULSE_COLON;
}

inline int findPresetIndex(const ClockConfig& config, const String& id) {
  for (size_t index = 0; index < config.presetCount; ++index) {
    if (config.presets[index].id == id) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

inline int findWidgetIndex(const ClockConfig& config, const String& id) {
  for (size_t index = 0; index < config.widgetCount; ++index) {
    if (config.widgets[index].id == id) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

inline int findFaceIndex(const ClockConfig& config, const String& id) {
  for (size_t index = 0; index < config.faceCount; ++index) {
    if (config.faces[index].id == id) {
      return static_cast<int>(index);
    }
  }
  return -1;
}
