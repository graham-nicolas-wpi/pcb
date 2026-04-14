#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

#include "src/calibration/ClockProtocol.h"
#include "src/display/LedMap.h"
#include "src/display/LogicalDisplay.h"
#include "src/hal/Buttons.h"
#include "src/hal/PixelDriver_NeoPixel.h"
#include "src/hal/Rtc_DS3231.h"
#include "src/settings/SettingsStore.h"
#include "src/ui/AppStateMachine.h"

namespace Pins {
static constexpr uint8_t kPixels = 5;
static constexpr uint8_t kBtn1 = 8;
static constexpr uint8_t kBtn2 = 9;
static constexpr uint8_t kBtn3 = 10;
}

namespace ProjectInfo {
static constexpr const char* kFirmwareName = "clock_firmware";
static constexpr const char* kFirmwareVersion = "0.3.0";
}

PixelDriver_NeoPixel gPixels(Pins::kPixels, kNumPixels);
LedMap gMap;
LogicalDisplay gDisplay(gPixels, gMap);
Buttons gButtons(Pins::kBtn1, Pins::kBtn2, Pins::kBtn3);
SettingsStore gSettings;
AppStateMachine gApp;
Rtc_DS3231 gRtc;
ClockProtocol gProtocol(Serial, gSettings, gApp, gRtc, ProjectInfo::kFirmwareName,
                        ProjectInfo::kFirmwareVersion);

static DateTime fallbackNow(uint32_t nowMs) {
  const uint32_t totalSeconds = nowMs / 1000UL;
  const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
  const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60UL) % 60UL);
  const uint8_t hours = static_cast<uint8_t>((totalSeconds / 3600UL) % 24UL);
  return DateTime(2026, 1, 1, hours, minutes, seconds);
}

static void renderIfRunning(uint32_t nowMs) {
  const DateTime now = gRtc.isPresent() ? gRtc.now() : fallbackNow(nowMs);
  gApp.render(gDisplay, gPixels, gSettings.settings(), now, nowMs);
}

static void handleButtons(uint32_t nowMs) {
  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN1)) {
    gApp.cycleModeForward();
    gSettings.setMode(gApp.mode());
    renderIfRunning(nowMs);
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN2)) {
    if (gApp.mode() == ClockMode::TIMER) {
      gSettings.setTimerPresetSeconds(gSettings.settings().timerPresetSeconds + 60UL);
      gApp.resetTimer(gSettings.settings().timerPresetSeconds);
      renderIfRunning(nowMs);
    } else if (gApp.mode() == ClockMode::STOPWATCH) {
      gApp.toggleStopwatch(nowMs);
      renderIfRunning(nowMs);
    } else {
      gApp.setMode(ClockMode::CLOCK);
      gSettings.setMode(gApp.mode());
      renderIfRunning(nowMs);
    }
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN3)) {
    if (gApp.mode() == ClockMode::TIMER) {
      uint32_t timerPresetSeconds = gSettings.settings().timerPresetSeconds;
      if (timerPresetSeconds >= 60UL) {
        timerPresetSeconds -= 60UL;
      } else {
        timerPresetSeconds = 0;
      }
      gSettings.setTimerPresetSeconds(timerPresetSeconds);
      gApp.resetTimer(gSettings.settings().timerPresetSeconds);
      renderIfRunning(nowMs);
    } else if (gApp.mode() == ClockMode::STOPWATCH) {
      gApp.resetStopwatch(nowMs);
      renderIfRunning(nowMs);
    } else {
      gApp.setMode(ClockMode::COLOR_DEMO);
      gSettings.setMode(gApp.mode());
      renderIfRunning(nowMs);
    }
  }
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
  gProtocol.begin(96U);

  if (gRtc.begin() && gRtc.lostPower()) {
    gRtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  renderIfRunning(millis());
  gProtocol.sendFirmwareInfo();
}

void loop() {
  const uint32_t nowMs = millis();

  gButtons.update();
  gProtocol.poll();
  handleButtons(nowMs);

  if (gProtocol.consumeRenderRequested()) {
    renderIfRunning(nowMs);
  }

  if (gApp.update(nowMs)) {
    renderIfRunning(nowMs);
  }

  if (gApp.consumeTimerDoneEvent()) {
    gProtocol.sendTimerDone();
    renderIfRunning(nowMs);
  }

  gSettings.tick(nowMs);
}
