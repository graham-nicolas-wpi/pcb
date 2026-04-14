#include "SegmentFrame.h"

namespace {
static constexpr uint8_t kDigitMask[10] = {
    0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110,
    0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01101111};

uint8_t hexNibble(const char value) {
  if (value >= '0' && value <= '9') {
    return static_cast<uint8_t>(value - '0');
  }
  if (value >= 'A' && value <= 'F') {
    return static_cast<uint8_t>(10 + (value - 'A'));
  }
  if (value >= 'a' && value <= 'f') {
    return static_cast<uint8_t>(10 + (value - 'a'));
  }
  return 255U;
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

bool SegmentFrame::parseHexByte(const char* text, uint8_t& value) {
  const uint8_t hi = hexNibble(text[0]);
  const uint8_t lo = hexNibble(text[1]);
  if (hi > 15U || lo > 15U) {
    return false;
  }
  value = static_cast<uint8_t>((hi << 4U) | lo);
  return true;
}

bool SegmentFrame::fromFrame24Payload(const char* payload) {
  if (payload == nullptr) {
    return false;
  }

  const size_t payloadLen = strlen(payload);
  if (payloadLen != kFrame24HexChars) {
    return false;
  }

  for (uint8_t logical = 0; logical < kLogicalCount; ++logical) {
    const char* cursor = payload + (static_cast<size_t>(logical) * 6U);
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    if (!parseHexByte(cursor, red) || !parseHexByte(cursor + 2, green) ||
        !parseHexByte(cursor + 4, blue)) {
      return false;
    }
    logicalColors[logical] = makeRgbColor(red, green, blue);
  }

  return true;
}
