#include <Arduino.h>
#include <Wire.h>

#include "display/LedMap.h"
#include "display/LogicalDisplay.h"
#include "hal/Buttons.h"
#include "hal/PixelDriver_NeoPixel.h"
#include "protocol/SerialProtocol.h"
#include "runtime/ClockRuntime.h"
#include "storage/SettingsStore.h"
#include "web/WebUiServer.h"

SettingsStore gStore;
LedMap gMap;
PixelDriver_NeoPixel* gPixels = nullptr;
Buttons* gButtons = nullptr;
LogicalDisplay* gDisplay = nullptr;
ClockRuntime* gRuntime = nullptr;
SerialProtocol* gSerialProtocol = nullptr;
WebUiServer* gWebUi = nullptr;

void setup() {
  Serial.begin(115200);
  delay(80);

  gStore.begin();

  const HardwareSettings& hardware = gStore.config().hardware;
  Wire.begin(hardware.rtcSda, hardware.rtcScl);

  gPixels = new PixelDriver_NeoPixel(hardware.pixelPin, hardware.pixelCount);
  gButtons = new Buttons(hardware.buttonPins[0], hardware.buttonPins[1], hardware.buttonPins[2]);
  gDisplay = new LogicalDisplay(*gPixels, gMap);
  gRuntime = new ClockRuntime(*gPixels, *gButtons, gMap, *gDisplay, gStore);
  gSerialProtocol = new SerialProtocol(Serial, gStore, *gRuntime);
  gWebUi = new WebUiServer(gStore, *gRuntime);

  gPixels->begin();
  gButtons->begin();
  gRuntime->begin();
  gSerialProtocol->begin();
  gWebUi->begin();

  Serial.println();
  Serial.println("OK BOOT ClockESP/1");
}

void loop() {
  const uint32_t nowMs = millis();
  gSerialProtocol->poll();
  gRuntime->update(nowMs);
  gWebUi->update();
}
