#pragma once

#include <Arduino.h>

struct RgbColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

inline RgbColor makeRgbColor(uint8_t r, uint8_t g, uint8_t b) {
  RgbColor color = {r, g, b};
  return color;
}

inline RgbColor scaleColor(const RgbColor& color, uint8_t scale) {
  return makeRgbColor(static_cast<uint8_t>((static_cast<uint16_t>(color.r) * scale) / 255U),
                      static_cast<uint8_t>((static_cast<uint16_t>(color.g) * scale) / 255U),
                      static_cast<uint8_t>((static_cast<uint16_t>(color.b) * scale) / 255U));
}

inline RgbColor blendColor(const RgbColor& a, const RgbColor& b, uint8_t mix) {
  const uint16_t inv = static_cast<uint16_t>(255U - mix);
  return makeRgbColor(static_cast<uint8_t>((a.r * inv + b.r * mix) / 255U),
                      static_cast<uint8_t>((a.g * inv + b.g * mix) / 255U),
                      static_cast<uint8_t>((a.b * inv + b.b * mix) / 255U));
}

inline RgbColor colorFromHue(uint8_t hue) {
  const uint8_t region = hue / 43U;
  const uint8_t remainder = static_cast<uint8_t>((hue - (region * 43U)) * 6U);
  const uint8_t q = static_cast<uint8_t>(255U - remainder);
  const uint8_t t = remainder;

  switch (region) {
    case 0:
      return makeRgbColor(255, t, 0);
    case 1:
      return makeRgbColor(q, 255, 0);
    case 2:
      return makeRgbColor(0, 255, t);
    case 3:
      return makeRgbColor(0, q, 255);
    case 4:
      return makeRgbColor(t, 0, 255);
    default:
      return makeRgbColor(255, 0, q);
  }
}
