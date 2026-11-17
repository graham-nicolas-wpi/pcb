#pragma once

#include <Arduino.h>

#include "../display/LogicalIds.h"
#include "../util/Color.h"

class SegmentFrame {
 public:
  static constexpr size_t kFrame24HexChars = static_cast<size_t>(kLogicalCount) * 6U;

  SegmentFrame();

  void clear();
  void setLogical(uint8_t logical, const RgbColor& color);
  void clearDigit(uint8_t digitIndex);
  void renderDigit(uint8_t digitIndex, uint8_t value, const RgbColor& color);
  void renderDigits(const uint8_t digits[4], const RgbColor colors[4], bool blankLeadingDigit);
  void setColon(bool topOn, bool bottomOn, const RgbColor& color);
  void setDecimal(bool on, const RgbColor& color);
  void toFrame24Payload(String& output) const;

  RgbColor logicalColors[kLogicalCount];

 private:
  static uint8_t digitBaseLogical(uint8_t digitIndex);
};
