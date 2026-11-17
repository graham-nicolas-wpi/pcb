#include "IWatchFace.h"

uint32_t IWatchFace::toColor(const PixelDriver_NeoPixel& pixels, const RgbColor& color) {
  return pixels.color(color.r, color.g, color.b);
}

uint32_t IWatchFace::toColorScaled(const PixelDriver_NeoPixel& pixels, const RgbColor& color,
                                   uint8_t scale) {
  const uint16_t r = (static_cast<uint16_t>(color.r) * scale) / 255U;
  const uint16_t g = (static_cast<uint16_t>(color.g) * scale) / 255U;
  const uint16_t b = (static_cast<uint16_t>(color.b) * scale) / 255U;
  return pixels.color(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                      static_cast<uint8_t>(b));
}

uint8_t IWatchFace::colonScale(const ClockSettings& settings) {
  if (settings.colonBrightness == 0U) return 0U;
  const uint16_t v =
      (static_cast<uint16_t>(settings.brightness) * settings.colonBrightness) / 255U;
  return v == 0U ? 1U : static_cast<uint8_t>(v);
}
