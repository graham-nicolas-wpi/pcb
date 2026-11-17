#pragma once

#include <Arduino.h>

#include "../render/SegmentFrame.h"
#include "../settings/SettingsStore.h"

struct FaceRenderState {
  SegmentFrame& frame;
  const ClockSettings& settings;
  uint8_t digits[4];
  bool blankLeadingDigit;
  bool colonTopOn;
  bool colonBottomOn;
  bool decimalOn;
  bool rainbowDigits;
  uint32_t animationStep;
  RgbColor digitColors[4];
  RgbColor colonColor;
  RgbColor decimalColor;
};

class IWatchFace {
 public:
  virtual ~IWatchFace() {}
  virtual void render(const FaceRenderState& state) = 0;

 protected:
  static RgbColor scaleColor(const RgbColor& color, uint8_t scale);
};
