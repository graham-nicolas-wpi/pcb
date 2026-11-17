#pragma once

#include <Arduino.h>

#include "SegmentFrame.h"

class LogicalDisplay;
class PixelDriver_NeoPixel;

class SegmentFrameRenderer {
 public:
  SegmentFrameRenderer(LogicalDisplay& display, PixelDriver_NeoPixel& pixels);

  void render(const SegmentFrame& frame);
  void testSegments(uint16_t msPerSegment = 180U);
  void testDigits(uint16_t msPerFrame = 300U);
  void testAll();

 private:
  LogicalDisplay& display_;
  PixelDriver_NeoPixel& pixels_;
};
