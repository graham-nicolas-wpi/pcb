#include "FrameCompiler.h"

namespace {
RgbColor rainbowColor(uint32_t nowMs, uint8_t offset) {
  return colorFromHue(static_cast<uint8_t>((nowMs / 12UL) + offset));
}

ColorPalette defaultPalette() {
  ColorPalette palette;
  palette.digit1 = makeRgbColor(255, 180, 96);
  palette.digit2 = makeRgbColor(255, 180, 96);
  palette.digit3 = makeRgbColor(255, 180, 96);
  palette.digit4 = makeRgbColor(255, 180, 96);
  palette.accent = makeRgbColor(64, 160, 255);
  palette.seconds = makeRgbColor(255, 255, 255);
  palette.timerHours = makeRgbColor(96, 255, 140);
  palette.timerMinutes = makeRgbColor(96, 255, 140);
  palette.timerSeconds = makeRgbColor(96, 255, 140);
  palette.stopwatchHours = makeRgbColor(255, 180, 96);
  palette.stopwatchMinutes = makeRgbColor(255, 180, 96);
  palette.stopwatchSeconds = makeRgbColor(255, 255, 255);
  return palette;
}
}

const PresetDefinition* FrameCompiler::activePreset(const ClockConfig& config) const {
  const int presetIndex = findPresetIndex(config, config.runtime.activePresetId);
  if (presetIndex >= 0) {
    return &config.presets[presetIndex];
  }
  return config.presetCount > 0 ? &config.presets[0] : nullptr;
}

const FaceDefinition* FrameCompiler::activeFaceForMode(const ClockConfig& config) const {
  const int activeIndex = findFaceIndex(config, config.runtime.activeFaceId);
  if (activeIndex >= 0 && config.faces[activeIndex].baseMode == config.runtime.mode) {
    return &config.faces[activeIndex];
  }
  for (size_t index = 0; index < config.faceCount; ++index) {
    if (config.faces[index].baseMode == config.runtime.mode) {
      return &config.faces[index];
    }
  }
  return config.faceCount > 0 ? &config.faces[0] : nullptr;
}

const WidgetDefinition* FrameCompiler::widgetById(const ClockConfig& config,
                                                  const String& id) const {
  const int widgetIndex = findWidgetIndex(config, id);
  return widgetIndex >= 0 ? &config.widgets[widgetIndex] : nullptr;
}

uint8_t FrameCompiler::pulseValue(uint32_t nowMs, uint8_t speed) const {
  const uint16_t period = 600U + static_cast<uint16_t>((255U - speed) * 12U);
  const uint16_t phase = static_cast<uint16_t>(nowMs % period);
  const uint16_t half = period / 2U;
  if (phase < half) {
    return static_cast<uint8_t>(80U + (175UL * phase) / half);
  }
  return static_cast<uint8_t>(255U - (175UL * (phase - half)) / half);
}

void FrameCompiler::applyEffect(RenderState& state, const ClockConfig& config, EffectMode effect,
                                uint32_t nowMs) const {
  (void)config;
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

void FrameCompiler::applyWidgets(RenderState& state, const ClockConfig& config,
                                 const FaceDefinition* face,
                                 const CompileContext& context) const {
  if (face == nullptr) {
    return;
  }

  for (uint8_t index = 0; index < face->widgetCount; ++index) {
    const WidgetDefinition* widget = widgetById(config, face->widgetIds[index]);
    if (widget == nullptr || !widget->enabled) {
      continue;
    }

    switch (widget->type) {
      case WidgetType::PULSE_COLON: {
        const uint8_t pulse = pulseValue(context.nowMs, widget->speed);
        state.colonColor = scaleColor(widget->primary, pulse);
        if (state.decimalOn) {
          state.decimalColor = scaleColor(widget->secondary, pulse);
        }
        break;
      }

      case WidgetType::RAINBOW_DIGITS: {
        for (uint8_t digit = 0; digit < 4; ++digit) {
          state.digitColors[digit] = rainbowColor(context.nowMs, static_cast<uint8_t>(digit * 52U));
        }
        break;
      }

      case WidgetType::TIMER_WARNING: {
        const uint32_t remainingSeconds = (context.timerRemainingMs + 999UL) / 1000UL;
        if (config.runtime.mode == ClockMode::TIMER &&
            remainingSeconds <= widget->thresholdSeconds) {
          const uint8_t pulse = pulseValue(context.nowMs, widget->speed);
          state.digitColors[2] = scaleColor(widget->primary, pulse);
          state.digitColors[3] = scaleColor(widget->secondary, pulse);
          state.colonColor = scaleColor(widget->primary, pulse);
        }
        break;
      }

      case WidgetType::SECONDS_SPARK: {
        if (config.runtime.mode == ClockMode::CLOCK ||
            config.runtime.mode == ClockMode::CLOCK_SECONDS) {
          state.decimalOn = true;
          state.decimalColor =
              (context.now.second() % 2U) == 0U ? widget->primary : widget->secondary;
        }
        break;
      }

      case WidgetType::ACCENT_SWEEP: {
        const RgbColor sweep =
            blendColor(widget->primary, widget->secondary, pulseValue(context.nowMs, widget->speed));
        state.colonColor = sweep;
        if (state.decimalOn) {
          state.decimalColor = sweep;
        }
        break;
      }
    }
  }
}

void FrameCompiler::buildRenderState(const ClockConfig& config, const CompileContext& context,
                                     RenderState& state) const {
  const PresetDefinition* preset = activePreset(config);
  const FaceDefinition* face = activeFaceForMode(config);
  const ColorPalette palette = preset != nullptr ? preset->palette : defaultPalette();

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

  switch (config.runtime.mode) {
    case ClockMode::CLOCK:
    case ClockMode::CLOCK_SECONDS: {
      uint8_t hours = context.now.hour();
      if (config.runtime.hourMode == HourMode::H12) {
        hours = static_cast<uint8_t>(hours % 12U);
        if (hours == 0U) {
          hours = 12U;
        }
      }
      state.digits[0] = static_cast<uint8_t>((hours / 10U) % 10U);
      state.digits[1] = static_cast<uint8_t>(hours % 10U);
      state.digits[2] = static_cast<uint8_t>((context.now.minute() / 10U) % 10U);
      state.digits[3] = static_cast<uint8_t>(context.now.minute() % 10U);
      state.blankLeadingDigit = (config.runtime.hourMode == HourMode::H12 && hours < 10U);
      switch (config.runtime.secondsMode) {
        case SecondsMode::BLINK:
          state.colonTopOn = state.colonBottomOn = ((context.now.second() % 2U) == 0U);
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
      TimerDisplayFormat format = config.runtime.timerFormat;
      if (face != nullptr && face->autoTimerFormat) {
        format = TimerDisplayFormat::AUTO;
      }
      if (format == TimerDisplayFormat::AUTO) {
        if (context.timerRemainingMs >= 3600000UL) {
          format = TimerDisplayFormat::HHMM;
        } else if (context.timerRemainingMs >= 60000UL) {
          format = TimerDisplayFormat::MMSS;
        } else {
          format = TimerDisplayFormat::SSCS;
        }
      }

      if (format == TimerDisplayFormat::HHMM) {
        const uint32_t totalMinutes = (context.timerRemainingMs + 59999UL) / 60000UL;
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
        const uint32_t totalSeconds = (context.timerRemainingMs + 999UL) / 1000UL;
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
        const uint32_t totalCentiseconds = context.timerRemainingMs / 10UL;
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
      if (context.stopwatchElapsedMs < 60000UL) {
        const uint32_t totalCentiseconds = context.stopwatchElapsedMs / 10UL;
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
      } else if (context.stopwatchElapsedMs < 3600000UL) {
        const uint32_t totalSeconds = context.stopwatchElapsedMs / 1000UL;
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
        const uint32_t totalMinutes = context.stopwatchElapsedMs / 60000UL;
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
      state.digitColors[0] = rainbowColor(context.nowMs, 0U);
      state.digitColors[1] = rainbowColor(context.nowMs, 48U);
      state.digitColors[2] = rainbowColor(context.nowMs, 96U);
      state.digitColors[3] = rainbowColor(context.nowMs, 144U);
      state.colonColor = rainbowColor(context.nowMs, 192U);
      state.decimalColor = rainbowColor(context.nowMs, 216U);
      state.decimalOn = true;
      break;
    }
  }

  if (face != nullptr && !face->showColon) {
    state.colonTopOn = false;
    state.colonBottomOn = false;
  }
  if (face != nullptr && !face->showDecimal && config.runtime.mode != ClockMode::TIMER &&
      config.runtime.mode != ClockMode::STOPWATCH &&
      config.runtime.mode != ClockMode::COLOR_DEMO) {
    state.decimalOn = false;
  }

  EffectMode effect = config.runtime.effect;
  if (preset != nullptr && preset->effect != EffectMode::NONE) {
    effect = preset->effect;
  }
  if (face != nullptr && face->effect != EffectMode::NONE) {
    effect = face->effect;
  }
  if (config.runtime.rainbowEnabled) {
    effect = EffectMode::RAINBOW;
  }

  applyEffect(state, config, effect, context.nowMs);
  applyWidgets(state, config, face, context);
}

void FrameCompiler::compile(const ClockConfig& config, const CompileContext& context,
                            SegmentFrame& frame) const {
  RenderState state;
  buildRenderState(config, context, state);

  RgbColor digitColors[4] = {
      scaleColor(state.digitColors[0], config.runtime.brightness),
      scaleColor(state.digitColors[1], config.runtime.brightness),
      scaleColor(state.digitColors[2], config.runtime.brightness),
      scaleColor(state.digitColors[3], config.runtime.brightness),
  };

  frame.clear();
  frame.renderDigits(state.digits, digitColors, state.blankLeadingDigit);
  frame.setColon(state.colonTopOn, state.colonBottomOn,
                 scaleColor(state.colonColor, config.runtime.colonBrightness));
  frame.setDecimal(state.decimalOn,
                   scaleColor(state.decimalColor, config.runtime.brightness));
}
