#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "../clock_firmware/src/calibration/CalibrationController.h"
#include "../clock_firmware/src/display/LedMap.h"
#include "../clock_firmware/src/display/LogicalDisplay.h"
#include "../clock_firmware/src/hal/Buttons.h"
#include "../clock_firmware/src/hal/PixelDriver_NeoPixel.h"

namespace Pins {
static constexpr uint8_t kPixels = 5;
static constexpr uint8_t kBtn1 = 8;
static constexpr uint8_t kBtn2 = 9;
static constexpr uint8_t kBtn3 = 10;
}

namespace ProjectInfo {
static constexpr const char* kFirmwareName = "clock_calibration_firmware";
static constexpr const char* kFirmwareVersion = "0.3.0";
}

PixelDriver_NeoPixel gPixels(Pins::kPixels, kNumPixels);
LedMap gMap;
LogicalDisplay gDisplay(gPixels, gMap);
Buttons gButtons(Pins::kBtn1, Pins::kBtn2, Pins::kBtn3);
CalibrationController gCalibration(Serial, gPixels, gMap, gDisplay);

String gLineBuffer;
size_t gLineMax = 96U;

static void handleButtons() {
  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN1)) {
    gCalibration.setCalibrationMode(true);
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN2)) {
    gCalibration.nextPhys(+1);
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN3)) {
    gCalibration.nextPhys(-1);
  }
}

static void pollSerial() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (gLineBuffer.length() > 0U) {
        gCalibration.handleCommand(gLineBuffer);
        gLineBuffer = "";
      }
      continue;
    }

    if (gLineBuffer.length() < gLineMax) {
      gLineBuffer += c;
    } else {
      gLineBuffer = "";
      Serial.println(F("ERR LINE TOO LONG"));
    }
  }
}

void setup() {
  Serial.begin(115200);

  gPixels.begin();
  gPixels.setBrightness(255);
  gButtons.begin();
  gMap.loadOrDefault();
  gCalibration.begin(true);

  gLineBuffer.reserve(gLineMax + 1U);

  Serial.print(F("OK HELLO ClockCal/1 NAME="));
  Serial.print(ProjectInfo::kFirmwareName);
  Serial.print(F(" FW="));
  Serial.print(ProjectInfo::kFirmwareVersion);
  Serial.print(F(" PIXELS="));
  Serial.print(kNumPixels);
  Serial.print(F(" LOGICAL="));
  Serial.println(static_cast<int>(LOGICAL_COUNT));
}

void loop() {
  gButtons.update();
  handleButtons();
  pollSerial();
}
