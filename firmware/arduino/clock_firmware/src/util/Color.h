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
