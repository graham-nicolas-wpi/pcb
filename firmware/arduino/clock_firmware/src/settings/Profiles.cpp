#include "Profiles.h"

namespace {
bool matches(const char* value, const char* expected) {
  return strcmp(value, expected) == 0;
}
}

bool applyPresetByName(const char* presetName, ClockSettings& settings) {
  if (matches(presetName, "RAINBOW")) {
    settings.digit1Color = makeRgbColor(255, 0, 0);
    settings.digit2Color = makeRgbColor(255, 120, 0);
    settings.digit3Color = makeRgbColor(0, 255, 0);
    settings.digit4Color = makeRgbColor(0, 120, 255);
    settings.accentColor = makeRgbColor(180, 0, 255);
    settings.secondsColor = makeRgbColor(0, 120, 255);
    settings.timerColor = makeRgbColor(180, 0, 255);
    settings.timerHoursColor = settings.timerColor;
    settings.timerMinutesColor = settings.timerColor;
    settings.timerSecondsColor = settings.timerColor;
    settings.stopwatchHoursColor = settings.digit1Color;
    settings.stopwatchMinutesColor = settings.digit2Color;
    settings.stopwatchSecondsColor = settings.secondsColor;
    settings.rainbowClockMode = true;
    return true;
  }

  if (matches(presetName, "PARTY")) {
    settings.digit1Color = makeRgbColor(255, 0, 255);
    settings.digit2Color = makeRgbColor(255, 0, 255);
    settings.digit3Color = makeRgbColor(255, 0, 255);
    settings.digit4Color = makeRgbColor(255, 0, 255);
    settings.accentColor = makeRgbColor(0, 255, 255);
    settings.secondsColor = makeRgbColor(255, 255, 0);
    settings.timerColor = makeRgbColor(255, 80, 80);
    settings.timerHoursColor = settings.timerColor;
    settings.timerMinutesColor = settings.timerColor;
    settings.timerSecondsColor = settings.timerColor;
    settings.stopwatchHoursColor = settings.digit1Color;
    settings.stopwatchMinutesColor = settings.digit2Color;
    settings.stopwatchSecondsColor = settings.secondsColor;
    settings.rainbowClockMode = false;
    return true;
  }

  return false;
}

bool applyColorTarget(const char* target, const RgbColor& newColor, ClockSettings& settings) {
  if (matches(target, "CLOCK")) {
    settings.digit1Color = newColor;
    settings.digit2Color = newColor;
    settings.digit3Color = newColor;
    settings.digit4Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "HOURS")) {
    settings.digit1Color = newColor;
    settings.digit2Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "MINUTES")) {
    settings.digit3Color = newColor;
    settings.digit4Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "DIGIT1")) {
    settings.digit1Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "DIGIT2")) {
    settings.digit2Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "DIGIT3")) {
    settings.digit3Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "DIGIT4")) {
    settings.digit4Color = newColor;
    settings.rainbowClockMode = false;
    return true;
  }

  if (matches(target, "ACCENT")) {
    settings.accentColor = newColor;
    return true;
  }

  if (matches(target, "SECONDS")) {
    settings.secondsColor = newColor;
    return true;
  }

  if (matches(target, "TIMER")) {
    settings.timerColor = newColor;
    settings.timerHoursColor = newColor;
    settings.timerMinutesColor = newColor;
    settings.timerSecondsColor = newColor;
    return true;
  }

  if (matches(target, "STOPWATCH")) {
    settings.stopwatchHoursColor = newColor;
    settings.stopwatchMinutesColor = newColor;
    settings.stopwatchSecondsColor = newColor;
    return true;
  }

  if (matches(target, "TIMER_HOURS")) {
    settings.timerHoursColor = newColor;
    return true;
  }

  if (matches(target, "TIMER_MINUTES")) {
    settings.timerMinutesColor = newColor;
    return true;
  }

  if (matches(target, "TIMER_SECONDS")) {
    settings.timerSecondsColor = newColor;
    return true;
  }

  if (matches(target, "STOPWATCH_HOURS")) {
    settings.stopwatchHoursColor = newColor;
    return true;
  }

  if (matches(target, "STOPWATCH_MINUTES")) {
    settings.stopwatchMinutesColor = newColor;
    return true;
  }

  if (matches(target, "STOPWATCH_SECONDS")) {
    settings.stopwatchSecondsColor = newColor;
    return true;
  }

  return false;
}
