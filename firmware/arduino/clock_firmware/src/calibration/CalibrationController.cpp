#include "CalibrationController.h"

#include "../display/LedMap.h"
#include "../display/LogicalDisplay.h"
#include "../hal/PixelDriver_NeoPixel.h"

CalibrationController::CalibrationController(Stream& serial, PixelDriver_NeoPixel& pixels,
                                             LedMap& map, LogicalDisplay& display)
    : mSerial(serial), mPixels(pixels), mMap(map), mDisplay(display) {}

void CalibrationController::begin(bool startInCalibration) {
  mCalibrationMode = startInCalibration;
  mCurPhys = 0;
  if (mCalibrationMode) {
    showCurrent(mPixels.color(255, 255, 255));
  } else {
    mPixels.clear();
    mPixels.show();
  }
}

void CalibrationController::setCalibrationMode(bool enabled) {
  mCalibrationMode = enabled;
  if (mCalibrationMode) {
    showCurrent(mPixels.color(255, 255, 255));
  } else {
    mPixels.clear();
    mPixels.show();
  }
}

bool CalibrationController::isCalibrationMode() const { return mCalibrationMode; }

void CalibrationController::setPhys(uint8_t p) {
  if (p >= kNumPixels) return;
  mCurPhys = p;
  if (mCalibrationMode) showCurrent(mPixels.color(255, 255, 255));
}

void CalibrationController::nextPhys(int8_t delta) {
  int16_t x = static_cast<int16_t>(mCurPhys) + delta;
  while (x < 0) x += kNumPixels;
  while (x >= kNumPixels) x -= kNumPixels;
  setPhys(static_cast<uint8_t>(x));
}

void CalibrationController::showCurrent(uint32_t color) {
  if (color == 0) color = mPixels.color(255, 255, 255);
  mPixels.clear();
  mPixels.setPixelColor(mCurPhys, color);
  mPixels.show();
}

void CalibrationController::sendHello() const {
  mSerial.print(F("OK HELLO ClockCal/1 FW=0.3.0 PIXELS="));
  mSerial.print(kNumPixels);
  mSerial.print(F(" LOGICAL="));
  mSerial.println(static_cast<int>(LOGICAL_COUNT));
}

void CalibrationController::replyOK(const __FlashStringHelper* tag) {
  mSerial.print(F("OK "));
  mSerial.println(tag);
}

void CalibrationController::replyOK(const char* tag) {
  mSerial.print(F("OK "));
  mSerial.println(tag);
}

void CalibrationController::replyERR(uint8_t code, const __FlashStringHelper* msg) {
  mSerial.print(F("ERR "));
  mSerial.print(code);
  mSerial.print(' ');
  mSerial.println(msg);
}

void CalibrationController::replyERR(uint8_t code, const char* msg) {
  mSerial.print(F("ERR "));
  mSerial.print(code);
  mSerial.print(' ');
  mSerial.println(msg);
}

void CalibrationController::sendInfo() const {
  mSerial.println(F("OK INFO BOARD=Leonardo PIN=5 PIXELS=31 PROTO=ClockCal/1 BTNS=8,9,10"));
}

void CalibrationController::sendStatus() const {
  mSerial.print(F("OK STATUS MODE=CAL CUR="));
  mSerial.print(mCurPhys);
  mSerial.print(F(" VALID="));
  mSerial.println(mMap.validateNoDuplicates() ? 1 : 0);
}

void CalibrationController::dumpMapCsv() {
  mSerial.print("MAP ");
  for (uint8_t i = 0; i < kNumPixels; ++i) {
    mSerial.print(mMap.physToLogical[i]);
    if (i + 1 < kNumPixels) mSerial.print(',');
  }
  mSerial.println();
}

void CalibrationController::testSegmentsWalk(uint16_t msPerSeg) {
  const bool wasCalibrationMode = mCalibrationMode;
  mCalibrationMode = false;
  for (uint8_t l = 0; l < LOGICAL_COUNT; ++l) {
    mDisplay.clear();
    mDisplay.setLogical(l, mPixels.color(255, 255, 255));
    mDisplay.show();
    delay(msPerSeg);
  }
  mCalibrationMode = wasCalibrationMode;
  if (mCalibrationMode) {
    showCurrent();
  }
}

void CalibrationController::testDigitsSequence(uint16_t msPerFrame) {
  const bool wasCalibrationMode = mCalibrationMode;
  mCalibrationMode = false;
  for (uint8_t n = 0; n < 10; ++n) {
    const bool colon = (n % 2) == 0;
    mDisplay.renderFourDigits(n, (n + 1) % 10, (n + 2) % 10, (n + 3) % 10, colon,
                              mPixels.color(255, 80, 20), mPixels.color(0, 80, 255));
    delay(msPerFrame);
  }
  mCalibrationMode = wasCalibrationMode;
  if (mCalibrationMode) {
    showCurrent();
  }
}

bool CalibrationController::handleCommand(const String& line) {
  if (line.length() == 0) {
    return false;
  }
  if (line == "HELLO" || line == "HELLO ClockCal/1" || line == "HELLO Clock/1") {
    sendHello();
    return true;
  }
  if (line == "INFO?" || line == "GET INFO" || line == "CAL INFO?" || line == "INFO CAL?") {
    sendInfo();
    return true;
  }
  if (line == "STATUS?" || line == "GET STATUS") {
    sendStatus();
    return true;
  }
  if (line == "MODE CAL") {
    setCalibrationMode(true);
    replyOK(F("MODE CAL"));
    return true;
  }
  if (line == "MODE RUN") {
    setCalibrationMode(false);
    replyOK(F("MODE RUN"));
    return true;
  }
  if (line == "CUR?") {
    mSerial.print("CUR ");
    mSerial.println(mCurPhys);
    return true;
  }
  if (line.startsWith("SET ")) {
    int p = line.substring(4).toInt();
    if (p < 0 || p >= kNumPixels) {
      replyERR(1, F("bad phys"));
      return true;
    }
    setPhys(static_cast<uint8_t>(p));
    replyOK(F("SET"));
    return true;
  }
  if (line == "NEXT") {
    nextPhys(+1);
    replyOK(F("NEXT"));
    return true;
  }
  if (line == "PREV") {
    nextPhys(-1);
    replyOK(F("PREV"));
    return true;
  }
  if (line.startsWith("ASSIGN ")) {
    int l = line.substring(7).toInt();
    if (l != 255 && (l < 0 || l >= LOGICAL_COUNT)) {
      replyERR(2, F("bad logical"));
      return true;
    }
    if (!mMap.assign(mCurPhys, static_cast<uint8_t>(l))) {
      replyERR(2, F("bad logical"));
      return true;
    }
    replyOK(F("ASSIGN"));
    return true;
  }
  if (line == "UNASSIGN_CUR") {
    mMap.assign(mCurPhys, LOG_UNUSED);
    replyOK(F("UNASSIGN_CUR"));
    return true;
  }
  if (line == "MAP?") {
    dumpMapCsv();
    return true;
  }
  if (line == "VALIDATE?") {
    mSerial.println(mMap.validateNoDuplicates() ? "OK VALIDATE PASS" : "OK VALIDATE FAIL");
    return true;
  }
  if (line == "SAVE") {
    if (!mMap.saveToEeprom()) {
      replyERR(3, F("duplicates or invalid map"));
      return true;
    }
    replyOK(F("SAVE"));
    return true;
  }
  if (line == "LOAD") {
    if (!mMap.loadFromEeprom()) {
      replyERR(4, F("no map"));
      return true;
    }
    replyOK(F("LOAD"));
    return true;
  }
  if (line == "TEST DIGITS") {
    testDigitsSequence();
    replyOK(F("TEST DIGITS"));
    return true;
  }
  if (line == "TEST SEGMENTS") {
    testSegmentsWalk();
    replyOK(F("TEST SEGMENTS"));
    return true;
  }
  if (line == "TEST ALL") {
    testSegmentsWalk(180);
    testDigitsSequence(300);
    replyOK(F("TEST ALL"));
    return true;
  }
  return false;
}
