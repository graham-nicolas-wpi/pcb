#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

#include "src/display/LedMap.h"
#include "src/display/LogicalDisplay.h"
#include "src/hal/Buttons.h"
#include "src/hal/PixelDriver_NeoPixel.h"
#include "src/hal/Rtc_DS3231.h"
#include "src/protocol/ClockRenderProtocol.h"
#include "src/render/SegmentFrameRenderer.h"
#include "src/settings/SettingsStore.h"
#include "src/runtime/RendererRuntime.h"
#include "src/ui/AppStateMachine.h"

namespace Pins {
static constexpr uint8_t kPixels = 5;
static constexpr uint8_t kBtn1 = 8;
static constexpr uint8_t kBtn2 = 9;
static constexpr uint8_t kBtn3 = 10;
}

namespace ProjectInfo {
static constexpr const char* kFirmwareName = "clock_firmware";
static constexpr const char* kFirmwareVersion = "0.4.0";
}

PixelDriver_NeoPixel gPixels(Pins::kPixels, kNumPixels);
LedMap gMap;
LogicalDisplay gDisplay(gPixels, gMap);
Buttons gButtons(Pins::kBtn1, Pins::kBtn2, Pins::kBtn3);
SettingsStore gSettings;
AppStateMachine gApp;
Rtc_DS3231 gRtc;
SegmentFrameRenderer gRenderer(gDisplay, gPixels);
RendererRuntime gRuntime(gButtons, gSettings, gApp, gRenderer);
ClockRenderProtocol gProtocol(Serial, gSettings, gApp, gRtc, gMap, gRuntime, gRenderer,
                              ProjectInfo::kFirmwareName, ProjectInfo::kFirmwareVersion);

static DateTime fallbackNow(uint32_t nowMs) {
  const uint32_t totalSeconds = nowMs / 1000UL;
  const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
  const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60UL) % 60UL);
  const uint8_t hours = static_cast<uint8_t>((totalSeconds / 3600UL) % 24UL);
  return DateTime(2026, 1, 1, hours, minutes, seconds);
}

static DateTime currentNow(uint32_t nowMs) {
  return gRtc.isPresent() ? gRtc.now() : fallbackNow(nowMs);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  gPixels.begin();
  gPixels.setBrightness(255);
  gButtons.begin();
  gMap.loadOrDefault();
  gSettings.begin();
  gApp.begin(gSettings.settings(), millis());
  gRuntime.begin(millis());
  gProtocol.begin(224U);

  if (gRtc.begin() && gRtc.lostPower()) {
    gRtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  gRuntime.requestRender();
  gRuntime.update(millis(), currentNow(millis()));
  gProtocol.sendHello();
}

void loop() {
  const uint32_t nowMs = millis();
  const DateTime now = currentNow(nowMs);

  gProtocol.poll();
  gRuntime.update(nowMs, now);
  gProtocol.flushEvents();
  gSettings.tick(nowMs);
}
