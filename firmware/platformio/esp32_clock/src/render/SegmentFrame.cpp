#include "SegmentFrame.h"

namespace {
static constexpr uint8_t kDigitMask[10] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
    0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};

char hexDigit(uint8_t value) {
  return static_cast<char>(value < 10U ? ('0' + value) : ('A' + (value - 10U)));
}
}

SegmentFrame::SegmentFrame() { clear(); }

void SegmentFrame::clear() {
  for (RgbColor& color : logicalColors) {
    color = makeRgbColor(0, 0, 0);
  }
}

void SegmentFrame::setLogical(uint8_t logical, const RgbColor& color) {
  if (logical >= kLogicalCount) {
    return;
  }
  logicalColors[logical] = color;
}

uint8_t SegmentFrame::digitBaseLogical(uint8_t digitIndex) {
  switch (digitIndex) {
    case 0:
      return D1_A;
    case 1:
      return D2_A;
    case 2:
      return D3_A;
    case 3:
      return D4_A;
    default:
      return D1_A;
  }
}

void SegmentFrame::clearDigit(uint8_t digitIndex) {
  if (digitIndex > 3U) {
    return;
  }
  const uint8_t base = digitBaseLogical(digitIndex);
  for (uint8_t segment = 0; segment < 7U; ++segment) {
    setLogical(base + segment, makeRgbColor(0, 0, 0));
  }
}

void SegmentFrame::renderDigit(uint8_t digitIndex, uint8_t value, const RgbColor& color) {
  if (digitIndex > 3U) {
    return;
  }
  const uint8_t base = digitBaseLogical(digitIndex);
  const uint8_t mask = kDigitMask[value % 10U];
  for (uint8_t segment = 0; segment < 7U; ++segment) {
    setLogical(base + segment,
               (mask & (1U << segment)) != 0U ? color : makeRgbColor(0, 0, 0));
  }
}

void SegmentFrame::renderDigits(const uint8_t digits[4], const RgbColor colors[4],
                                bool blankLeadingDigit) {
  if (blankLeadingDigit) {
    clearDigit(0);
  } else if (digits[0] <= 9U) {
    renderDigit(0, digits[0], colors[0]);
  } else {
    clearDigit(0);
  }

  for (uint8_t index = 1; index < 4U; ++index) {
    if (digits[index] <= 9U) {
      renderDigit(index, digits[index], colors[index]);
    } else {
      clearDigit(index);
    }
  }
}

void SegmentFrame::setColon(bool topOn, bool bottomOn, const RgbColor& color) {
  setLogical(COLON_TOP, topOn ? color : makeRgbColor(0, 0, 0));
  setLogical(COLON_BOTTOM, bottomOn ? color : makeRgbColor(0, 0, 0));
}

void SegmentFrame::setDecimal(bool on, const RgbColor& color) {
  setLogical(DECIMAL, on ? color : makeRgbColor(0, 0, 0));
}

void SegmentFrame::toFrame24Payload(String& output) const {
  output = "";
  output.reserve(kFrame24HexChars);
  for (uint8_t logical = 0; logical < kLogicalCount; ++logical) {
    const RgbColor& color = logicalColors[logical];
    output += hexDigit(static_cast<uint8_t>(color.r >> 4U));
    output += hexDigit(static_cast<uint8_t>(color.r & 0x0FU));
    output += hexDigit(static_cast<uint8_t>(color.g >> 4U));
    output += hexDigit(static_cast<uint8_t>(color.g & 0x0FU));
    output += hexDigit(static_cast<uint8_t>(color.b >> 4U));
    output += hexDigit(static_cast<uint8_t>(color.b & 0x0FU));
  }
}
