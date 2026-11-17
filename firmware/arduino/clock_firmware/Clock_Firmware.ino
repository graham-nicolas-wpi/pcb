#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <avr/power.h>

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

// Boards flashed through the chip's factory USB (DFU) bootloader keep the
// factory CKDIV8 fuse, which boots the CPU at 16 MHz / 8. Arduino's main()
// calls initVariant() before USB starts, so undo the divider there.
// Harmless on boards with the Arduino (Caterina) bootloader.
void initVariant() { clock_prescale_set(clock_div_1); }

// The ESP32 host talks to us on the hardware UART (D0/D1, J2 pins 5/4) and
// clockWinder can still talk over USB. Read from whichever one starts a
// line (stick to it until the newline), and send replies/events to both.
class DualStream : public Stream {
 public:
  DualStream(Stream& host, Stream& usb) : host_(host), usb_(usb) {}
  int available() override {
    Stream* s = pick();
    return s ? s->available() : 0;
  }
  int read() override {
    Stream* s = pick();
    if (!s) return -1;
    const int c = s->read();
    current_ = (c == '\n' || c < 0) ? nullptr : s;
    return c;
  }
  int peek() override {
    Stream* s = pick();
    return s ? s->peek() : -1;
  }
  size_t write(uint8_t c) override {
    host_.write(c);
    usb_.write(c);
    return 1;
  }
  void flush() override {}

 private:
  Stream* pick() {
    if (current_) return current_;
    if (host_.available()) return &host_;
    if (usb_.available()) return &usb_;
    return nullptr;
  }
  Stream& host_;
  Stream& usb_;
  Stream* current_ = nullptr;
};

DualStream gLink(Serial1, Serial);

namespace Pins {
static constexpr uint8_t kPixels = 5;
static constexpr uint8_t kBtn1 = 8;
static constexpr uint8_t kBtn2 = 9;
static constexpr uint8_t kBtn3 = 10;
}

namespace ProjectInfo {
static constexpr const char* kFirmwareName = "clock_firmware";
static constexpr const char* kFirmwareVersion = "0.5.0";
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
ClockRenderProtocol gProtocol(gLink, gSettings, gApp, gRtc, gMap, gRuntime, gRenderer,
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
  Serial1.begin(115200);  // ESP32 host link on D0 (RX) / D1 (TX)
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
