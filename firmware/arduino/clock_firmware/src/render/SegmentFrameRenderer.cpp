#include "SegmentFrameRenderer.h"

#include "../display/LogicalDisplay.h"
#include "../hal/PixelDriver_NeoPixel.h"

SegmentFrameRenderer::SegmentFrameRenderer(LogicalDisplay& display, PixelDriver_NeoPixel& pixels)
    : display_(display), pixels_(pixels) {}

void SegmentFrameRenderer::render(const SegmentFrame& frame) {
  display_.clear();
  for (uint8_t logical = 0; logical < kLogicalCount; ++logical) {
    const RgbColor& color = frame.logicalColors[logical];
    display_.setLogical(logical, pixels_.color(color.r, color.g, color.b));
  }
  display_.show();
}

void SegmentFrameRenderer::testSegments(uint16_t msPerSegment) {
  SegmentFrame frame;
  for (uint8_t logical = 0; logical < kLogicalCount; ++logical) {
    frame.clear();
    frame.setLogical(logical, makeRgbColor(255, 255, 255));
    render(frame);
    delay(msPerSegment);
  }
}

void SegmentFrameRenderer::testDigits(uint16_t msPerFrame) {
  SegmentFrame frame;
  const RgbColor digitColors[4] = {
      makeRgbColor(255, 80, 20),
      makeRgbColor(255, 80, 20),
      makeRgbColor(255, 80, 20),
      makeRgbColor(255, 80, 20),
  };

  for (uint8_t value = 0; value < 10U; ++value) {
    const uint8_t digits[4] = {
        value,
        static_cast<uint8_t>((value + 1U) % 10U),
        static_cast<uint8_t>((value + 2U) % 10U),
        static_cast<uint8_t>((value + 3U) % 10U),
    };
    frame.clear();
    frame.renderDigits(digits, digitColors, false);
    frame.setColon((value % 2U) == 0U, (value % 2U) == 0U, makeRgbColor(0, 80, 255));
    render(frame);
    delay(msPerFrame);
  }
}

void SegmentFrameRenderer::testAll() {
  testSegments(180U);
  testDigits(300U);
}
