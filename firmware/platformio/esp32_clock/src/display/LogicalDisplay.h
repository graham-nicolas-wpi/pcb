#pragma once

#include <stdint.h>

#include "LogicalIds.h"

class PixelDriver_NeoPixel;
class LedMap;

class LogicalDisplay {
 public:
  LogicalDisplay(PixelDriver_NeoPixel& pixels, LedMap& map);

  void clear();
  void show();
  void setLogical(uint8_t logical, uint32_t color);
  void setColon(bool topOn, bool bottomOn, uint32_t color);
  void setDecimal(bool on, uint32_t color);
  void clearDigit(uint8_t digitIndex);
  void renderDigit(uint8_t digitIndex, uint8_t value, uint32_t color);
  void renderDigits(const uint8_t digits[4], const uint32_t colors[4], bool blankLeadingDigit);
  void renderFourDigits(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, bool colonOn,
                        uint32_t digitColor, uint32_t colonColor);

 private:
  uint8_t digitBaseLogical(uint8_t digitIndex) const;

  PixelDriver_NeoPixel& pixels_;
  LedMap& map_;
};
