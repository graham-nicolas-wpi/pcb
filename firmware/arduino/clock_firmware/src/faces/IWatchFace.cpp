#include "IWatchFace.h"

RgbColor IWatchFace::scaleColor(const RgbColor& color, uint8_t scale) {
  const uint16_t r = (static_cast<uint16_t>(color.r) * scale) / 255U;
  const uint16_t g = (static_cast<uint16_t>(color.g) * scale) / 255U;
  const uint16_t b = (static_cast<uint16_t>(color.b) * scale) / 255U;
  return makeRgbColor(static_cast<uint8_t>(r), static_cast<uint8_t>(g),
                      static_cast<uint8_t>(b));
}
