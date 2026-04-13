#pragma once
#include <Arduino.h>
#include "LogicalIds.h"

class PixelDriver_NeoPixel;
class LedMap;
class LogicalDisplay;

class CalibrationController {
public:
  CalibrationController(PixelDriver_NeoPixel& pixels, LedMap& map, LogicalDisplay& display);
  void begin();
  void processSerial();
  bool handleCommand(const String& line);
  void setCalibrationMode(bool enabled);
  bool isCalibrationMode() const;
  void nextPhys(int8_t delta);
private:
  bool readLine();
  void processLine(const String& line);
  void dumpMapCsv();
  void replyOK(const char* tag);
  void replyERR(uint8_t code, const char* msg);
  void setPhys(uint8_t p);
  void showCurrent(uint32_t color = 0);
  void testSegmentsWalk(uint16_t msPerSeg = 250);
  void testDigitsSequence(uint16_t msPerFrame = 400);

  PixelDriver_NeoPixel& mPixels;
  LedMap& mMap;
  LogicalDisplay& mDisplay;
  uint8_t mCurPhys = 0;
  bool mCalibrationMode = true;
  String mLine;
};
