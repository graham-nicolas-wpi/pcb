#include "CalibrationController.h"
#include "LedMap.h"
#include "PixelDriver_NeoPixel.h"
#include "LogicalDisplay.h"

CalibrationController::CalibrationController(PixelDriver_NeoPixel& pixels, LedMap& map, LogicalDisplay& display)
  : mPixels(pixels), mMap(map), mDisplay(display) {}

void CalibrationController::begin() {
  mCalibrationMode = true;
  mCurPhys = 0;
  showCurrent(mPixels.color(255, 255, 255));
}

void CalibrationController::setCalibrationMode(bool enabled) {
  mCalibrationMode = enabled;
  if (mCalibrationMode) showCurrent(mPixels.color(255, 255, 255));
  else { mPixels.clear(); mPixels.show(); }
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

bool CalibrationController::readLine() {
  while (Serial.available()) {
    char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;
    if (c == '\n') return true;
    mLine += c;
    if (mLine.length() > 120) { mLine = ""; return false; }
  }
  return false;
}

void CalibrationController::replyOK(const char* tag) {
  Serial.print("OK ");
  Serial.println(tag);
}

void CalibrationController::replyERR(uint8_t code, const char* msg) {
  Serial.print("ERR ");
  Serial.print(code);
  Serial.print(' ');
  Serial.println(msg);
}

void CalibrationController::dumpMapCsv() {
  Serial.print("MAP ");
  for (uint8_t i = 0; i < kNumPixels; ++i) {
    Serial.print(mMap.physToLogical[i]);
    if (i + 1 < kNumPixels) Serial.print(',');
  }
  Serial.println();
}

void CalibrationController::testSegmentsWalk(uint16_t msPerSeg) {
  setCalibrationMode(false);
  for (uint8_t l = 0; l < LOGICAL_COUNT; ++l) {
    mDisplay.clear();
    mDisplay.setLogical(l, mPixels.color(255, 255, 255));
    mDisplay.show();
    delay(msPerSeg);
  }
}

void CalibrationController::testDigitsSequence(uint16_t msPerFrame) {
  setCalibrationMode(false);
  for (uint8_t n = 0; n < 10; ++n) {
    const bool colon = (n % 2) == 0;
    mDisplay.renderFourDigits(n, (n + 1) % 10, (n + 2) % 10, (n + 3) % 10, colon,
                              mPixels.color(255, 80, 20), mPixels.color(0, 80, 255));
    delay(msPerFrame);
  }
}

bool CalibrationController::handleCommand(const String& line) {
  if (line.length() == 0) return false;
  if (line.startsWith("HELLO")) {
    Serial.print("OK HELLO ClockCal/1 FW=0.1.0 PIXELS=");
    Serial.print(kNumPixels);
    Serial.print(" LOGICAL=");
    Serial.println(static_cast<int>(LOGICAL_COUNT));
    return true;
  }
  if (line == "INFO?") { Serial.println("OK INFO BOARD=Leonardo PIN=5 PIXELS=31 PROTO=ClockCal/1 BTNS=8,9,10"); return true; }
  if (line == "MODE CAL") { setCalibrationMode(true); replyOK("MODE"); return true; }
  if (line == "MODE RUN") { setCalibrationMode(false); replyOK("MODE"); return true; }
  if (line == "CUR?") { Serial.print("CUR "); Serial.println(mCurPhys); return true; }
  if (line.startsWith("SET ")) {
    int p = line.substring(4).toInt();
    if (p < 0 || p >= kNumPixels) { replyERR(1, "bad phys"); return true; }
    setPhys(static_cast<uint8_t>(p));
    replyOK("SET");
    return true;
  }
  if (line == "NEXT") { nextPhys(+1); replyOK("NEXT"); return true; }
  if (line == "PREV") { nextPhys(-1); replyOK("PREV"); return true; }
  if (line.startsWith("ASSIGN ")) {
    int l = line.substring(7).toInt();
    if (l != 255 && (l < 0 || l >= LOGICAL_COUNT)) { replyERR(2, "bad logical"); return true; }
    mMap.physToLogical[mCurPhys] = static_cast<uint8_t>(l);
    mMap.buildInverse();
    replyOK("ASSIGN");
    return true;
  }
  if (line == "UNASSIGN_CUR") { mMap.physToLogical[mCurPhys] = LOG_UNUSED; mMap.buildInverse(); replyOK("UNASSIGN_CUR"); return true; }
  if (line == "MAP?") { dumpMapCsv(); return true; }
  if (line == "VALIDATE?") { Serial.println(mMap.validateNoDuplicates() ? "OK VALIDATE PASS" : "OK VALIDATE FAIL"); return true; }
  if (line == "SAVE") { if (!mMap.saveToEeprom()) { replyERR(3, "duplicates or invalid map"); return true; } replyOK("SAVE"); return true; }
  if (line == "LOAD") { if (!mMap.loadFromEeprom()) { replyERR(4, "no map"); return true; } replyOK("LOAD"); return true; }
  if (line == "TEST DIGITS") { testDigitsSequence(); replyOK("TEST DIGITS"); return true; }
  if (line == "TEST SEGMENTS") { testSegmentsWalk(); replyOK("TEST SEGMENTS"); return true; }
  if (line == "TEST ALL") { testSegmentsWalk(180); testDigitsSequence(300); replyOK("TEST ALL"); return true; }
  return false;
}

void CalibrationController::processSerial() {
  if (readLine()) {
    handleCommand(mLine);
    mLine = "";
  }
}
