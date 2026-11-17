#pragma once

#include <Arduino.h>

namespace ProjectConfig {
static constexpr const char* kFirmwareVersion = "0.3.0";
static constexpr uint8_t kDefaultPixelCount = 31;
// Legacy direct-drive fields (kept only for old config files; not used by the host).
static constexpr uint8_t kDefaultPixelPin = 18;
static constexpr uint8_t kDefaultButton1Pin = 4;
static constexpr uint8_t kDefaultButton2Pin = 5;
static constexpr uint8_t kDefaultButton3Pin = 19;
static constexpr uint8_t kDefaultRtcSdaPin = 21;
static constexpr uint8_t kDefaultRtcSclPin = 22;
// UART link to the 32U4 renderer. GPIO16/17 are wired to the PSRAM on
// ESP32-WROVER modules, so the link uses GPIO26 (TX) and GPIO27 (RX).
// TX -> clock J2 pin 5 (D0/RX), RX <- clock J2 pin 4 (D1/TX) through a
// 1k / 2k divider (the 32U4 drives 5 V).
static constexpr uint8_t kDefaultRendererTxPin = 26;
static constexpr uint8_t kDefaultRendererRxPin = 27;

// KY-040 scroll knob, moved here from the 32U4 (its D0/D1 are now the UART).
static constexpr uint8_t kKnobClkPin = 32;
static constexpr uint8_t kKnobDtPin = 33;
static constexpr uint8_t kKnobSwPin = 25;

// clockWinder over WiFi: line-based ClockRender/1 commands on this TCP port.
static constexpr uint16_t kWinderTcpPort = 3333;
static constexpr uint32_t kDefaultRendererBaud = 115200UL;
static constexpr uint16_t kRenderRefreshMs = 40;
static constexpr uint32_t kSaveDebounceMs = 1500UL;
static constexpr uint32_t kStatusPollMs = 1000UL;
static constexpr uint32_t kRendererHelloRetryMs = 1500UL;
static constexpr uint32_t kRendererRtcPollMs = 15000UL;
static constexpr uint32_t kRendererMapPollMs = 30000UL;
static constexpr uint32_t kRendererControlRefreshMs = 4000UL;
static constexpr uint32_t kNtpPollMs = 60000UL;
static constexpr size_t kMaxPresets = 12U;
static constexpr size_t kMaxWidgets = 12U;
static constexpr size_t kMaxFaces = 12U;
static constexpr size_t kMaxFaceWidgets = 6U;
}
