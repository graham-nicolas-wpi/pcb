#include "LogicalDisplay.h"

#include "../hal/PixelDriver_NeoPixel.h"
#include "LedMap.h"

namespace {
static const uint8_t kDigitMask[10] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
    0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};
}

LogicalDisplay::LogicalDisplay(PixelDriver_NeoPixel& pixels, LedMap& map)
    : pixels_(pixels), map_(map) {}

void LogicalDisplay::clear() { pixels_.clear(); }

void LogicalDisplay::show() { pixels_.show(); }

void LogicalDisplay::setLogical(uint8_t logical, uint32_t color) {
  if (logical >= LOGICAL_COUNT) {
    return;
  }
  const uint8_t physical = map_.logicalToPhys[logical];
  if (physical == LOG_UNUSED || physical >= kNumPixels) {
    return;
  }
  pixels_.setPixelColor(physical, color);
}

void LogicalDisplay::setColon(bool topOn, bool bottomOn, uint32_t color) {
  setLogical(COLON_TOP, topOn ? color : 0);
  setLogical(COLON_BOTTOM, bottomOn ? color : 0);
}

void LogicalDisplay::setDecimal(bool on, uint32_t color) {
  setLogical(DECIMAL, on ? color : 0);
}

uint8_t LogicalDisplay::digitBaseLogical(uint8_t digitIndex) const {
  switch (digitIndex) {
    case 0: return D1_A;
    case 1: return D2_A;
    case 2: return D3_A;
    case 3: return D4_A;
    default: return D1_A;
  }
}

void LogicalDisplay::clearDigit(uint8_t digitIndex) {
  if (digitIndex > 3) {
    return;
  }
  const uint8_t base = digitBaseLogical(digitIndex);
  for (uint8_t segment = 0; segment < 7; ++segment) {
    setLogical(base + segment, 0);
  }
}

void LogicalDisplay::renderDigit(uint8_t digitIndex, uint8_t value, uint32_t color) {
  if (digitIndex > 3) {
    return;
  }
  const uint8_t base = digitBaseLogical(digitIndex);
  const uint8_t mask = kDigitMask[value % 10U];
  for (uint8_t segment = 0; segment < 7; ++segment) {
    setLogical(base + segment, (mask & (1U << segment)) ? color : 0);
  }
}

void LogicalDisplay::renderDigits(const uint8_t digits[4], const uint32_t colors[4],
                                  bool blankLeadingDigit) {
  if (blankLeadingDigit) {
    clearDigit(0);
  } else if (digits[0] <= 9U) {
    renderDigit(0, digits[0], colors[0]);
  } else {
    clearDigit(0);
  }

  for (uint8_t index = 1; index < 4; ++index) {
    if (digits[index] <= 9U) {
      renderDigit(index, digits[index], colors[index]);
    } else {
      clearDigit(index);
    }
  }
}

void LogicalDisplay::renderFourDigits(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4,
                                      bool colonOn, uint32_t digitColor, uint32_t colonColor) {
  clear();
  renderDigit(0, d1, digitColor);
  renderDigit(1, d2, digitColor);
  renderDigit(2, d3, digitColor);
  renderDigit(3, d4, digitColor);
  setColon(colonOn, colonOn, colonColor);
  show();
}
