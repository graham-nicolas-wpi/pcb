#pragma once

#include <RTClib.h>

#include "../model/ClockConfig.h"
#include "SegmentFrame.h"

struct CompileContext {
  DateTime now;
  uint32_t nowMs = 0;
  uint32_t timerRemainingMs = 0;
  bool timerRunning = false;
  uint32_t stopwatchElapsedMs = 0;
};

class FrameCompiler {
 public:
  void compile(const ClockConfig& config, const CompileContext& context,
               SegmentFrame& frame) const;

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

  const PresetDefinition* activePreset(const ClockConfig& config) const;
  const FaceDefinition* activeFaceForMode(const ClockConfig& config) const;
  const WidgetDefinition* widgetById(const ClockConfig& config, const String& id) const;
  void buildRenderState(const ClockConfig& config, const CompileContext& context,
                        RenderState& state) const;
  void applyEffect(RenderState& state, const ClockConfig& config, EffectMode effect,
                   uint32_t nowMs) const;
  void applyWidgets(RenderState& state, const ClockConfig& config,
                    const FaceDefinition* face, const CompileContext& context) const;
  uint8_t pulseValue(uint32_t nowMs, uint8_t speed) const;
};
