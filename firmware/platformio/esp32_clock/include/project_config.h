#pragma once

#include <Arduino.h>

namespace ProjectConfig {
static constexpr uint8_t kDefaultPixelCount = 31;
static constexpr uint8_t kDefaultPixelPin = 18;
static constexpr uint8_t kDefaultButton1Pin = 25;
static constexpr uint8_t kDefaultButton2Pin = 26;
static constexpr uint8_t kDefaultButton3Pin = 27;
static constexpr uint8_t kDefaultRtcSdaPin = 21;
static constexpr uint8_t kDefaultRtcSclPin = 22;
static constexpr uint16_t kRenderRefreshMs = 40;
static constexpr uint32_t kSaveDebounceMs = 1500UL;
static constexpr uint32_t kStatusPollMs = 1000UL;
static constexpr size_t kMaxPresets = 12U;
static constexpr size_t kMaxWidgets = 12U;
static constexpr size_t kMaxFaces = 12U;
static constexpr size_t kMaxFaceWidgets = 6U;
}
