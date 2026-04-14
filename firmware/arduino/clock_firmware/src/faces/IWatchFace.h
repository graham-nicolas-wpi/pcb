#pragma once

#include <Arduino.h>

#include "../display/LogicalDisplay.h"
#include "../hal/PixelDriver_NeoPixel.h"
#include "../settings/SettingsStore.h"

struct FaceRenderState {
  LogicalDisplay& display;
  PixelDriver_NeoPixel& pixels;
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
  static uint32_t toColor(const PixelDriver_NeoPixel& pixels, const RgbColor& color);
  static uint32_t toColorScaled(const PixelDriver_NeoPixel& pixels, const RgbColor& color,
                                uint8_t scale);
};
