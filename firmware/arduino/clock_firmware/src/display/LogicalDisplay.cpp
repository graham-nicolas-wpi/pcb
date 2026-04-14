#include "LogicalDisplay.h"
#include "LedMap.h"
#include "../hal/PixelDriver_NeoPixel.h"

static const uint8_t kDigitMask[10] = {
  0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
  0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111
};

LogicalDisplay::LogicalDisplay(PixelDriver_NeoPixel& pixels, LedMap& map) : mPixels(pixels), mMap(map) {}
void LogicalDisplay::clear() { mPixels.clear(); }
void LogicalDisplay::show() { mPixels.show(); }

void LogicalDisplay::setLogical(uint8_t logical, uint32_t color) {
  if (logical >= LOGICAL_COUNT) return;
  uint8_t phys = mMap.logicalToPhys[logical];
  if (phys == LOG_UNUSED || phys >= kNumPixels) return;
  mPixels.setPixelColor(phys, color);
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

void LogicalDisplay::setColon(bool topOn, bool bottomOn, uint32_t color) {
  setLogical(COLON_TOP, topOn ? color : 0);
  setLogical(COLON_BOTTOM, bottomOn ? color : 0);
}

void LogicalDisplay::setDecimal(bool on, uint32_t color) {
  setLogical(DECIMAL, on ? color : 0);
}

void LogicalDisplay::clearDigit(uint8_t digitIndex) {
  if (digitIndex > 3) return;
  const uint8_t base = digitBaseLogical(digitIndex);
  for (uint8_t s = 0; s < 7; ++s) {
    setLogical(base + s, 0);
  }
}

void LogicalDisplay::renderDigit(uint8_t digitIndex, uint8_t value, uint32_t color) {
  if (digitIndex > 3) return;
  const uint8_t base = digitBaseLogical(digitIndex);
  const uint8_t mask = kDigitMask[value % 10];
  for (uint8_t s = 0; s < 7; ++s) setLogical(base + s, (mask & (1 << s)) ? color : 0);
}

void LogicalDisplay::renderDigits(const uint8_t digits[4], const uint32_t colors[4],
                                  bool blankLeadingDigit) {
  if (blankLeadingDigit) {
    clearDigit(0);
  } else {
    if (digits[0] <= 9U) {
      renderDigit(0, digits[0], colors[0]);
    } else {
      clearDigit(0);
    }
  }
  if (digits[1] <= 9U) {
    renderDigit(1, digits[1], colors[1]);
  } else {
    clearDigit(1);
  }
  if (digits[2] <= 9U) {
    renderDigit(2, digits[2], colors[2]);
  } else {
    clearDigit(2);
  }
  if (digits[3] <= 9U) {
    renderDigit(3, digits[3], colors[3]);
  } else {
    clearDigit(3);
  }
}

void LogicalDisplay::renderFourDigits(uint8_t d1, uint8_t d2, uint8_t d3, uint8_t d4, bool colonOn, uint32_t digitColor, uint32_t colonColor) {
  clear();
  renderDigit(0, d1, digitColor);
  renderDigit(1, d2, digitColor);
  renderDigit(2, d3, digitColor);
  renderDigit(3, d4, digitColor);
  setColon(colonOn, colonOn, colonColor);
  show();
}
