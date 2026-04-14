#pragma once
#include <Arduino.h>

#include "../display/LogicalIds.h"

class PixelDriver_NeoPixel;
class LedMap;
class LogicalDisplay;

class CalibrationController {
public:
  CalibrationController(Stream& serial, PixelDriver_NeoPixel& pixels, LedMap& map,
                        LogicalDisplay& display);
  void begin(bool startInCalibration = false);
  bool handleCommand(const String& line);
  void setCalibrationMode(bool enabled);
  bool isCalibrationMode() const;
  void nextPhys(int8_t delta);
  void sendHello() const;
  void sendInfo() const;
  void sendStatus() const;
private:
  void dumpMapCsv();
  void replyOK(const char* tag);
  void replyOK(const __FlashStringHelper* tag);
  void replyERR(uint8_t code, const char* msg);
  void replyERR(uint8_t code, const __FlashStringHelper* msg);
  void setPhys(uint8_t p);
  void showCurrent(uint32_t color = 0);
  void testSegmentsWalk(uint16_t msPerSeg = 250);
  void testDigitsSequence(uint16_t msPerFrame = 400);

  Stream& mSerial;
  PixelDriver_NeoPixel& mPixels;
  LedMap& mMap;
  LogicalDisplay& mDisplay;
  uint8_t mCurPhys = 0;
  bool mCalibrationMode = false;
};
