#include "ClockRuntime.h"

#include "../../include/project_config.h"

namespace {
RgbColor rainbowColor(uint32_t nowMs, uint8_t offset) {
  return colorFromHue(static_cast<uint8_t>((nowMs / 12UL) + offset));
}
}

ClockRuntime::ClockRuntime(PixelDriver_NeoPixel& pixels, Buttons& buttons, LedMap& map,
                           LogicalDisplay& display, SettingsStore& store)
    : pixels_(pixels), buttons_(buttons), map_(map), display_(display), store_(store) {}

void ClockRuntime::begin() {
  pixels_.setBrightness(config().runtime.brightness);
  applyMapping();
  rtcPresent_ = rtc_.begin();
  rtcLostPower_ = rtcPresent_ ? rtc_.lostPower() : false;
  timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  timerRunning_ = false;
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  syncFaceToMode();
  markDirty(false);
}

void ClockRuntime::reloadFromConfig() {
  applyMapping();
  timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  timerRunning_ = false;
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  pixels_.setBrightness(config().runtime.brightness);
  syncFaceToMode();
  markDirty(false);
}

bool ClockRuntime::saveConfigNow(String* error) {
  persistPending_ = false;
  return store_.save(error);
}

bool ClockRuntime::loadConfigNow(String* error) {
  if (!store_.load(error)) {
    return false;
  }
  reloadFromConfig();
  return true;
}

const ClockConfig& ClockRuntime::config() const { return store_.config(); }

ClockConfig& ClockRuntime::mutableConfig() { return store_.config(); }

const PresetDefinition* ClockRuntime::activePreset() const {
  const int presetIndex = findPresetIndex(config(), config().runtime.activePresetId);
  if (presetIndex >= 0) {
    return &config().presets[presetIndex];
  }
  return config().presetCount > 0 ? &config().presets[0] : nullptr;
}

const FaceDefinition* ClockRuntime::activeFaceForMode() const {
  const int activeIndex = findFaceIndex(config(), config().runtime.activeFaceId);
  if (activeIndex >= 0 && config().faces[activeIndex].baseMode == config().runtime.mode) {
    return &config().faces[activeIndex];
  }
  for (size_t index = 0; index < config().faceCount; ++index) {
    if (config().faces[index].baseMode == config().runtime.mode) {
      return &config().faces[index];
    }
  }
  return config().faceCount > 0 ? &config().faces[0] : nullptr;
}

const WidgetDefinition* ClockRuntime::widgetById(const String& id) const {
  const int widgetIndex = findWidgetIndex(config(), id);
  return widgetIndex >= 0 ? &config().widgets[widgetIndex] : nullptr;
}

void ClockRuntime::markDirty(bool persist) {
  renderDirty_ = true;
  if (persist) {
    persistPending_ = true;
    lastSaveRequestMs_ = millis();
  }
}

void ClockRuntime::syncFaceToMode() {
  const FaceDefinition* face = activeFaceForMode();
  if (face != nullptr) {
    mutableConfig().runtime.activeFaceId = face->id;
    if (face->presetId.length() > 0) {
      mutableConfig().runtime.activePresetId = face->presetId;
    }
  }
}

void ClockRuntime::applyMapping() {
  map_.loadFromArray(config().mapping.physToLogical, kNumPixels);
}

void ClockRuntime::update(uint32_t nowMs) {
  buttons_.update();
  handleButtons(nowMs);
  updateTimerState(nowMs);

  if (persistPending_ && (nowMs - lastSaveRequestMs_) >= ProjectConfig::kSaveDebounceMs) {
    saveConfigNow(nullptr);
  }

  renderIfNeeded(nowMs);
}

void ClockRuntime::handleButtons(uint32_t nowMs) {
  if (buttons_.wasShortPressed(Buttons::ButtonId::BTN1)) {
    const uint8_t next = (static_cast<uint8_t>(config().runtime.mode) + 1U) % 5U;
    setMode(static_cast<ClockMode>(next));
  }

  if (buttons_.wasShortPressed(Buttons::ButtonId::BTN2)) {
    if (config().runtime.mode == ClockMode::TIMER) {
      setTimerPresetSeconds(config().runtime.timerPresetSeconds + 60UL);
    } else if (config().runtime.mode == ClockMode::STOPWATCH) {
      if (stopwatchRunning_) {
        stopStopwatch(nowMs);
      } else {
        startStopwatch(nowMs);
      }
    } else {
      setActivePreset(config().presetCount > 1 ? config().presets[1].id : config().runtime.activePresetId);
    }
  }

  if (buttons_.wasShortPressed(Buttons::ButtonId::BTN3)) {
    if (config().runtime.mode == ClockMode::TIMER) {
      const uint32_t seconds = config().runtime.timerPresetSeconds >= 60UL
                                   ? (config().runtime.timerPresetSeconds - 60UL)
                                   : 0UL;
      setTimerPresetSeconds(seconds);
    } else if (config().runtime.mode == ClockMode::STOPWATCH) {
      resetStopwatch(nowMs);
    } else {
      setEffect(config().runtime.effect == EffectMode::RAINBOW ? EffectMode::NONE
                                                               : EffectMode::RAINBOW);
    }
  }
}

void ClockRuntime::updateTimerState(uint32_t nowMs) {
  if (timerRunning_) {
    const uint32_t elapsed = nowMs - timerStartedMs_;
    if (elapsed >= timerStartedRemainingMs_) {
      timerRunning_ = false;
      timerRemainingMs_ = 0;
      renderDirty_ = true;
    } else {
      const uint32_t newRemaining = timerStartedRemainingMs_ - elapsed;
      if (newRemaining != timerRemainingMs_) {
        timerRemainingMs_ = newRemaining;
        renderDirty_ = true;
      }
    }
  }
}

void ClockRuntime::renderIfNeeded(uint32_t nowMs) {
  if (!renderDirty_ && (nowMs - lastRenderMs_) < ProjectConfig::kRenderRefreshMs) {
    return;
  }
  lastRenderMs_ = nowMs;
  renderDirty_ = false;

  if (config().runtime.calibrationMode) {
    renderCalibration();
  } else {
    renderRegular(nowMs);
  }
}

void ClockRuntime::renderCalibration() {
  pixels_.clear();
  pixels_.setPixelColor(config().runtime.calibrationCursor, pixels_.color(255, 255, 255));
  pixels_.show();
}

DateTime ClockRuntime::fallbackNow(uint32_t nowMs) const {
  const uint32_t totalSeconds = nowMs / 1000UL;
  const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
  const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60UL) % 60UL);
  const uint8_t hours = static_cast<uint8_t>((totalSeconds / 3600UL) % 24UL);
  return DateTime(2026, 1, 1, hours, minutes, seconds);
}

DateTime ClockRuntime::currentTime(uint32_t nowMs) const {
  return rtcPresent_ ? rtc_.now() : fallbackNow(nowMs);
}

uint32_t ClockRuntime::stopwatchElapsedMs(uint32_t nowMs) const {
  if (!stopwatchRunning_) {
    return stopwatchAccumulatedMs_;
  }
  return stopwatchAccumulatedMs_ + (nowMs - stopwatchStartedMs_);
}

uint8_t ClockRuntime::pulseValue(uint32_t nowMs, uint8_t speed) const {
  const uint16_t period = 600U + static_cast<uint16_t>((255U - speed) * 12U);
  const uint16_t phase = static_cast<uint16_t>(nowMs % period);
  const uint16_t half = period / 2U;
  if (phase < half) {
    return static_cast<uint8_t>(80U + (175UL * phase) / half);
  }
  return static_cast<uint8_t>(255U - (175UL * (phase - half)) / half);
}

uint32_t ClockRuntime::toColor(const RgbColor& color) const {
  return pixels_.color(color.r, color.g, color.b);
}

void ClockRuntime::applyEffect(RenderState& state, EffectMode effect, uint32_t nowMs) const {
  if (effect == EffectMode::NONE) {
    return;
  }
  if (effect == EffectMode::RAINBOW) {
    for (uint8_t index = 0; index < 4; ++index) {
      state.digitColors[index] = rainbowColor(nowMs, static_cast<uint8_t>(index * 32U));
    }
    state.colonColor = rainbowColor(nowMs, 172U);
    state.decimalColor = rainbowColor(nowMs, 212U);
    return;
  }

  if (effect == EffectMode::PULSE) {
    const uint8_t pulse = pulseValue(nowMs, 160U);
    for (RgbColor& color : state.digitColors) {
      color = scaleColor(color, pulse);
    }
    state.colonColor = scaleColor(state.colonColor, pulse);
    state.decimalColor = scaleColor(state.decimalColor, pulse);
    return;
  }

  for (uint8_t index = 0; index < 4; ++index) {
    state.digitColors[index] = rainbowColor(nowMs + 250UL, static_cast<uint8_t>(index * 48U));
  }
  state.colonColor = scaleColor(rainbowColor(nowMs, 220U), pulseValue(nowMs, 220U));
  state.decimalColor = rainbowColor(nowMs, 160U);
}

void ClockRuntime::applyWidgets(RenderState& state, const FaceDefinition* face, uint32_t nowMs) const {
  if (face == nullptr) {
    return;
  }

  for (uint8_t index = 0; index < face->widgetCount; ++index) {
    const WidgetDefinition* widget = widgetById(face->widgetIds[index]);
    if (widget == nullptr || !widget->enabled) {
      continue;
    }

    switch (widget->type) {
      case WidgetType::PULSE_COLON: {
        const uint8_t pulse = pulseValue(nowMs, widget->speed);
        state.colonColor = scaleColor(widget->primary, pulse);
        if (state.decimalOn) {
          state.decimalColor = scaleColor(widget->secondary, pulse);
        }
        break;
      }

      case WidgetType::RAINBOW_DIGITS: {
        for (uint8_t digit = 0; digit < 4; ++digit) {
          state.digitColors[digit] = rainbowColor(nowMs, static_cast<uint8_t>(digit * 52U));
        }
        break;
      }

      case WidgetType::TIMER_WARNING: {
        if (config().runtime.mode == ClockMode::TIMER &&
            timerRemainingSeconds() <= widget->thresholdSeconds) {
          const uint8_t pulse = pulseValue(nowMs, widget->speed);
          state.digitColors[2] = scaleColor(widget->primary, pulse);
          state.digitColors[3] = scaleColor(widget->secondary, pulse);
          state.colonColor = scaleColor(widget->primary, pulse);
        }
        break;
      }

      case WidgetType::SECONDS_SPARK: {
        if (config().runtime.mode == ClockMode::CLOCK || config().runtime.mode == ClockMode::CLOCK_SECONDS) {
          const DateTime now = currentTime(nowMs);
          state.decimalOn = true;
          state.decimalColor = (now.second() % 2U) == 0U ? widget->primary : widget->secondary;
        }
        break;
      }

      case WidgetType::ACCENT_SWEEP: {
        const RgbColor sweep = blendColor(widget->primary, widget->secondary, pulseValue(nowMs, widget->speed));
        state.colonColor = sweep;
        if (state.decimalOn) {
          state.decimalColor = sweep;
        }
        break;
      }
    }
  }
}

void ClockRuntime::buildRenderState(RenderState& state, uint32_t nowMs) const {
  const PresetDefinition* preset = activePreset();
  const FaceDefinition* face = activeFaceForMode();
  const ColorPalette palette =
      preset != nullptr ? preset->palette : config().presets[0].palette;

  state.digits[0] = 0;
  state.digits[1] = 0;
  state.digits[2] = 0;
  state.digits[3] = 0;
  state.digitColors[0] = palette.digit1;
  state.digitColors[1] = palette.digit2;
  state.digitColors[2] = palette.digit3;
  state.digitColors[3] = palette.digit4;
  state.blankLeadingDigit = false;
  state.colonTopOn = true;
  state.colonBottomOn = true;
  state.decimalOn = false;
  state.colonColor = palette.accent;
  state.decimalColor = palette.seconds;

  switch (config().runtime.mode) {
    case ClockMode::CLOCK:
    case ClockMode::CLOCK_SECONDS: {
      DateTime now = currentTime(nowMs);
      uint8_t hours = now.hour();
      if (config().runtime.hourMode == HourMode::H12) {
        hours = static_cast<uint8_t>(hours % 12U);
        if (hours == 0U) {
          hours = 12U;
        }
      }
      state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
      state.digits[1] = static_cast<uint8_t>(hours % 10U);
      state.digits[2] = static_cast<uint8_t>((now.minute() / 10U) % 10U);
      state.digits[3] = static_cast<uint8_t>(now.minute() % 10U);
      state.blankLeadingDigit = (config().runtime.hourMode == HourMode::H12 && hours < 10U);
      switch (config().runtime.secondsMode) {
        case SecondsMode::BLINK:
          state.colonTopOn = state.colonBottomOn = ((now.second() % 2U) == 0U);
          break;
        case SecondsMode::ON:
          state.colonTopOn = state.colonBottomOn = true;
          break;
        case SecondsMode::OFF:
          state.colonTopOn = state.colonBottomOn = false;
          break;
      }
      break;
    }

    case ClockMode::TIMER: {
      TimerDisplayFormat format = config().runtime.timerFormat;
      if (face != nullptr && face->autoTimerFormat) {
        format = TimerDisplayFormat::AUTO;
      }
      if (format == TimerDisplayFormat::AUTO) {
        if (timerRemainingMs_ >= 3600000UL) {
          format = TimerDisplayFormat::HHMM;
        } else if (timerRemainingMs_ >= 60000UL) {
          format = TimerDisplayFormat::MMSS;
        } else {
          format = TimerDisplayFormat::SSCS;
        }
      }

      if (format == TimerDisplayFormat::HHMM) {
        const uint32_t totalMinutes = (timerRemainingMs_ + 59999UL) / 60000UL;
        const uint16_t hours = static_cast<uint16_t>((totalMinutes / 60UL) % 100UL);
        const uint8_t minutes = static_cast<uint8_t>(totalMinutes % 60UL);
        state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(hours % 10U);
        state.digits[2] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(minutes % 10U);
        state.digitColors[0] = palette.timerHours;
        state.digitColors[1] = palette.timerHours;
        state.digitColors[2] = palette.timerMinutes;
        state.digitColors[3] = palette.timerMinutes;
        state.decimalOn = false;
      } else if (format == TimerDisplayFormat::MMSS) {
        const uint32_t totalSeconds = timerRemainingSeconds();
        const uint16_t minutes = static_cast<uint16_t>((totalSeconds / 60UL) % 100UL);
        const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
        state.digits[0] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(minutes % 10U);
        state.digits[2] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(seconds % 10U);
        state.digitColors[0] = palette.timerMinutes;
        state.digitColors[1] = palette.timerMinutes;
        state.digitColors[2] = palette.timerSeconds;
        state.digitColors[3] = palette.timerSeconds;
        state.decimalOn = false;
      } else {
        const uint32_t totalCentiseconds = timerRemainingMs_ / 10UL;
        const uint8_t seconds = static_cast<uint8_t>((totalCentiseconds / 100UL) % 100UL);
        const uint8_t centiseconds = static_cast<uint8_t>(totalCentiseconds % 100UL);
        state.digits[0] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(seconds % 10U);
        state.digits[2] = static_cast<uint8_t>((centiseconds / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(centiseconds % 10U);
        state.digitColors[0] = palette.timerSeconds;
        state.digitColors[1] = palette.timerSeconds;
        state.digitColors[2] = palette.timerSeconds;
        state.digitColors[3] = palette.timerSeconds;
        state.colonTopOn = false;
        state.colonBottomOn = false;
        state.decimalOn = true;
        state.decimalColor = palette.timerSeconds;
      }
      break;
    }

    case ClockMode::STOPWATCH: {
      const uint32_t elapsedMs = stopwatchElapsedMs(nowMs);
      if (elapsedMs < 60000UL) {
        const uint32_t totalCentiseconds = elapsedMs / 10UL;
        const uint8_t seconds = static_cast<uint8_t>((totalCentiseconds / 100UL) % 100UL);
        const uint8_t centiseconds = static_cast<uint8_t>(totalCentiseconds % 100UL);
        state.digits[0] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(seconds % 10U);
        state.digits[2] = static_cast<uint8_t>((centiseconds / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(centiseconds % 10U);
        state.digitColors[0] = palette.stopwatchSeconds;
        state.digitColors[1] = palette.stopwatchSeconds;
        state.digitColors[2] = palette.stopwatchSeconds;
        state.digitColors[3] = palette.stopwatchSeconds;
        state.colonTopOn = false;
        state.colonBottomOn = false;
        state.decimalOn = true;
        state.decimalColor = palette.stopwatchSeconds;
      } else if (elapsedMs < 3600000UL) {
        const uint32_t totalSeconds = elapsedMs / 1000UL;
        const uint16_t minutes = static_cast<uint16_t>((totalSeconds / 60UL) % 100UL);
        const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
        state.digits[0] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(minutes % 10U);
        state.digits[2] = static_cast<uint8_t>((seconds / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(seconds % 10U);
        state.digitColors[0] = palette.stopwatchMinutes;
        state.digitColors[1] = palette.stopwatchMinutes;
        state.digitColors[2] = palette.stopwatchSeconds;
        state.digitColors[3] = palette.stopwatchSeconds;
      } else {
        const uint32_t totalMinutes = elapsedMs / 60000UL;
        const uint16_t hours = static_cast<uint16_t>((totalMinutes / 60UL) % 100UL);
        const uint8_t minutes = static_cast<uint8_t>(totalMinutes % 60UL);
        state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
        state.digits[1] = static_cast<uint8_t>(hours % 10U);
        state.digits[2] = static_cast<uint8_t>((minutes / 10U) % 10U);
        state.digits[3] = static_cast<uint8_t>(minutes % 10U);
        state.digitColors[0] = palette.stopwatchHours;
        state.digitColors[1] = palette.stopwatchHours;
        state.digitColors[2] = palette.stopwatchMinutes;
        state.digitColors[3] = palette.stopwatchMinutes;
      }
      break;
    }

    case ClockMode::COLOR_DEMO: {
      state.digits[0] = 1;
      state.digits[1] = 2;
      state.digits[2] = 3;
      state.digits[3] = 4;
      state.digitColors[0] = rainbowColor(nowMs, 0U);
      state.digitColors[1] = rainbowColor(nowMs, 48U);
      state.digitColors[2] = rainbowColor(nowMs, 96U);
      state.digitColors[3] = rainbowColor(nowMs, 144U);
      state.colonColor = rainbowColor(nowMs, 192U);
      state.decimalColor = rainbowColor(nowMs, 216U);
      state.decimalOn = true;
      break;
    }
  }

  if (face != nullptr && !face->showColon) {
    state.colonTopOn = false;
    state.colonBottomOn = false;
  }
  if (face != nullptr && !face->showDecimal && config().runtime.mode != ClockMode::TIMER &&
      config().runtime.mode != ClockMode::STOPWATCH && config().runtime.mode != ClockMode::COLOR_DEMO) {
    state.decimalOn = false;
  }

  EffectMode effect = config().runtime.effect;
  if (preset != nullptr && preset->effect != EffectMode::NONE) {
    effect = preset->effect;
  }
  if (face != nullptr && face->effect != EffectMode::NONE) {
    effect = face->effect;
  }
  if (config().runtime.rainbowEnabled) {
    effect = EffectMode::RAINBOW;
  }

  applyEffect(state, effect, nowMs);
  applyWidgets(state, face, nowMs);
}

void ClockRuntime::renderRegular(uint32_t nowMs) {
  RenderState state;
  buildRenderState(state, nowMs);

  uint32_t digitColors[4] = {
      toColor(state.digitColors[0]),
      toColor(state.digitColors[1]),
      toColor(state.digitColors[2]),
      toColor(state.digitColors[3]),
  };
  display_.clear();
  display_.renderDigits(state.digits, digitColors, state.blankLeadingDigit);
  const RgbColor colonScaled = scaleColor(state.colonColor, config().runtime.colonBrightness);
  const RgbColor decimalScaled = scaleColor(state.decimalColor, config().runtime.colonBrightness);
  display_.setColon(state.colonTopOn, state.colonBottomOn, toColor(colonScaled));
  display_.setDecimal(state.decimalOn, toColor(decimalScaled));
  display_.show();
}

void ClockRuntime::writeInfoJson(JsonObject root) const {
  root["board"] = "ESP32-WROOM-32E";
  root["name"] = CLOCK_DEVICE_NAME;
  root["pixels"] = config().hardware.pixelCount;
  root["pixelPin"] = config().hardware.pixelPin;
  JsonArray buttons = root.createNestedArray("buttons");
  for (uint8_t pin : config().hardware.buttonPins) {
    buttons.add(pin);
  }
  root["protocol"] = "ClockESP/1";
}

void ClockRuntime::writeStatusJson(JsonObject root, uint32_t nowMs) const {
  root["mode"] = clockModeName(config().runtime.mode);
  root["hourMode"] = hourModeName(config().runtime.hourMode);
  root["secondsMode"] = secondsModeName(config().runtime.secondsMode);
  root["timerFormat"] = timerFormatName(config().runtime.timerFormat);
  root["effect"] = effectModeName(config().runtime.effect);
  root["brightness"] = config().runtime.brightness;
  root["colonBrightness"] = config().runtime.colonBrightness;
  root["timerPresetSeconds"] = config().runtime.timerPresetSeconds;
  root["timerRemainingSeconds"] = timerRemainingSeconds();
  root["timerRunning"] = timerRunning_;
  root["stopwatchRunning"] = stopwatchRunning_;
  root["stopwatchElapsedMs"] = stopwatchElapsedMs(nowMs);
  root["calibrationMode"] = config().runtime.calibrationMode;
  root["calibrationCursor"] = config().runtime.calibrationCursor;
  root["activePresetId"] = config().runtime.activePresetId;
  root["activeFaceId"] = config().runtime.activeFaceId;
  root["mappingValid"] = validateMapping();
  root["rtcPresent"] = rtcPresent_;
  root["rtcLostPower"] = rtcLostPower_;
}

void ClockRuntime::writeRtcJson(JsonObject root, uint32_t nowMs) const {
  if (!rtcPresent_) {
    root["present"] = false;
    root["summary"] = "missing";
    return;
  }
  const DateTime now = currentTime(nowMs);
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u", now.year(), now.month(),
           now.day(), now.hour(), now.minute(), now.second());
  root["present"] = true;
  root["lostPower"] = rtcLostPower_;
  root["summary"] = buffer;
}

void ClockRuntime::setMode(ClockMode mode) {
  mutableConfig().runtime.mode = mode;
  syncFaceToMode();
  markDirty();
}

void ClockRuntime::setHourMode(HourMode mode) {
  mutableConfig().runtime.hourMode = mode;
  markDirty();
}

void ClockRuntime::setSecondsMode(SecondsMode mode) {
  mutableConfig().runtime.secondsMode = mode;
  markDirty();
}

void ClockRuntime::setTimerFormat(TimerDisplayFormat format) {
  mutableConfig().runtime.timerFormat = format;
  markDirty();
}

void ClockRuntime::setBrightness(uint8_t brightness) {
  mutableConfig().runtime.brightness = brightness;
  pixels_.setBrightness(brightness);
  markDirty();
}

void ClockRuntime::setColonBrightness(uint8_t brightness) {
  mutableConfig().runtime.colonBrightness = brightness;
  markDirty();
}

void ClockRuntime::setEffect(EffectMode effect) {
  mutableConfig().runtime.effect = effect;
  markDirty();
}

bool ClockRuntime::setActivePreset(const String& id) {
  if (findPresetIndex(config(), id) < 0) {
    return false;
  }
  mutableConfig().runtime.activePresetId = id;
  markDirty();
  return true;
}

bool ClockRuntime::setActiveFace(const String& id) {
  if (findFaceIndex(config(), id) < 0) {
    return false;
  }
  mutableConfig().runtime.activeFaceId = id;
  markDirty();
  return true;
}

bool ClockRuntime::setColorTarget(const String& target, const RgbColor& color) {
  const int presetIndex = findPresetIndex(config(), config().runtime.activePresetId);
  if (presetIndex < 0) {
    return false;
  }
  PresetDefinition& preset = mutableConfig().presets[presetIndex];
  if (target == "CLOCK") {
    preset.palette.digit1 = color;
    preset.palette.digit2 = color;
    preset.palette.digit3 = color;
    preset.palette.digit4 = color;
  } else if (target == "HOURS") {
    preset.palette.digit1 = color;
    preset.palette.digit2 = color;
  } else if (target == "MINUTES") {
    preset.palette.digit3 = color;
    preset.palette.digit4 = color;
  } else if (target == "DIGIT1") {
    preset.palette.digit1 = color;
  } else if (target == "DIGIT2") {
    preset.palette.digit2 = color;
  } else if (target == "DIGIT3") {
    preset.palette.digit3 = color;
  } else if (target == "DIGIT4") {
    preset.palette.digit4 = color;
  } else if (target == "ACCENT") {
    preset.palette.accent = color;
  } else if (target == "SECONDS") {
    preset.palette.seconds = color;
  } else if (target == "TIMER") {
    preset.palette.timerHours = color;
    preset.palette.timerMinutes = color;
    preset.palette.timerSeconds = color;
  } else if (target == "TIMER_HOURS") {
    preset.palette.timerHours = color;
  } else if (target == "TIMER_MINUTES") {
    preset.palette.timerMinutes = color;
  } else if (target == "TIMER_SECONDS") {
    preset.palette.timerSeconds = color;
  } else if (target == "STOPWATCH") {
    preset.palette.stopwatchHours = color;
    preset.palette.stopwatchMinutes = color;
    preset.palette.stopwatchSeconds = color;
  } else if (target == "STOPWATCH_HOURS") {
    preset.palette.stopwatchHours = color;
  } else if (target == "STOPWATCH_MINUTES") {
    preset.palette.stopwatchMinutes = color;
  } else if (target == "STOPWATCH_SECONDS") {
    preset.palette.stopwatchSeconds = color;
  } else {
    return false;
  }
  markDirty();
  return true;
}

void ClockRuntime::setTimerPresetSeconds(uint32_t seconds) {
  mutableConfig().runtime.timerPresetSeconds = seconds;
  timerRemainingMs_ = seconds * 1000UL;
  timerRunning_ = false;
  markDirty();
}

void ClockRuntime::startTimer(uint32_t nowMs) {
  setMode(ClockMode::TIMER);
  if (timerRemainingMs_ == 0) {
    timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  }
  timerRunning_ = timerRemainingMs_ > 0;
  timerStartedMs_ = nowMs;
  timerStartedRemainingMs_ = timerRemainingMs_;
  renderDirty_ = true;
}

void ClockRuntime::stopTimer(uint32_t nowMs) {
  if (!timerRunning_) {
    return;
  }
  const uint32_t elapsed = nowMs - timerStartedMs_;
  timerRemainingMs_ = elapsed >= timerStartedRemainingMs_ ? 0 : (timerStartedRemainingMs_ - elapsed);
  timerRunning_ = false;
  renderDirty_ = true;
}

void ClockRuntime::resetTimer() {
  timerRunning_ = false;
  timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  markDirty(false);
}

void ClockRuntime::startStopwatch(uint32_t nowMs) {
  setMode(ClockMode::STOPWATCH);
  if (!stopwatchRunning_) {
    stopwatchRunning_ = true;
    stopwatchStartedMs_ = nowMs;
    renderDirty_ = true;
  }
}

void ClockRuntime::stopStopwatch(uint32_t nowMs) {
  if (!stopwatchRunning_) {
    return;
  }
  stopwatchAccumulatedMs_ += nowMs - stopwatchStartedMs_;
  stopwatchRunning_ = false;
  renderDirty_ = true;
}

void ClockRuntime::resetStopwatch(uint32_t nowMs) {
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  stopwatchStartedMs_ = nowMs;
  renderDirty_ = true;
}

void ClockRuntime::setCalibrationMode(bool enabled) {
  mutableConfig().runtime.calibrationMode = enabled;
  markDirty();
}

void ClockRuntime::nextCalibration(int8_t delta) {
  int16_t value = static_cast<int16_t>(config().runtime.calibrationCursor) + delta;
  while (value < 0) value += kNumPixels;
  while (value >= kNumPixels) value -= kNumPixels;
  mutableConfig().runtime.calibrationCursor = static_cast<uint8_t>(value);
  renderDirty_ = true;
}

bool ClockRuntime::setCalibrationCursor(uint8_t physical) {
  if (physical >= kNumPixels) {
    return false;
  }
  mutableConfig().runtime.calibrationCursor = physical;
  renderDirty_ = true;
  return true;
}

bool ClockRuntime::assignLogical(uint8_t logical) {
  if (!map_.assign(config().runtime.calibrationCursor, logical)) {
    return false;
  }
  map_.copyToArray(mutableConfig().mapping.physToLogical, kNumPixels);
  markDirty();
  return true;
}

void ClockRuntime::unassignCurrent() {
  map_.assign(config().runtime.calibrationCursor, LOG_UNUSED);
  map_.copyToArray(mutableConfig().mapping.physToLogical, kNumPixels);
  markDirty();
}

bool ClockRuntime::validateMapping() const { return map_.validateNoDuplicates(); }

void ClockRuntime::testSegments(uint16_t msPerSegment) {
  const bool wasCalibration = config().runtime.calibrationMode;
  mutableConfig().runtime.calibrationMode = false;
  for (uint8_t logical = 0; logical < kLogicalCount; ++logical) {
    display_.clear();
    display_.setLogical(logical, pixels_.color(255, 255, 255));
    display_.show();
    delay(msPerSegment);
  }
  mutableConfig().runtime.calibrationMode = wasCalibration;
  renderDirty_ = true;
}

void ClockRuntime::testDigits(uint16_t msPerFrame) {
  const bool wasCalibration = config().runtime.calibrationMode;
  mutableConfig().runtime.calibrationMode = false;
  for (uint8_t value = 0; value < 10; ++value) {
    display_.renderFourDigits(value, (value + 1U) % 10U, (value + 2U) % 10U,
                              (value + 3U) % 10U, (value % 2U) == 0U,
                              pixels_.color(255, 120, 40), pixels_.color(0, 120, 255));
    delay(msPerFrame);
  }
  mutableConfig().runtime.calibrationMode = wasCalibration;
  renderDirty_ = true;
}

void ClockRuntime::testAll() {
  testSegments(160);
  testDigits(260);
}

bool ClockRuntime::setRtc(const DateTime& value) {
  if (!rtcPresent_) {
    return false;
  }
  rtc_.adjust(value);
  rtcLostPower_ = false;
  renderDirty_ = true;
  return true;
}

bool ClockRuntime::rtcPresent() const { return rtcPresent_; }

bool ClockRuntime::rtcLostPower() const { return rtcLostPower_; }

uint32_t ClockRuntime::timerRemainingSeconds() const {
  return (timerRemainingMs_ + 999UL) / 1000UL;
}

bool ClockRuntime::timerRunning() const { return timerRunning_; }

bool ClockRuntime::stopwatchRunning() const { return stopwatchRunning_; }

uint8_t ClockRuntime::calibrationCursor() const { return config().runtime.calibrationCursor; }

bool ClockRuntime::calibrationMode() const { return config().runtime.calibrationMode; }
