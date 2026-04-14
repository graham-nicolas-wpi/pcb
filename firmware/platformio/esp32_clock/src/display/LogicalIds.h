#pragma once

#include <stdint.h>

constexpr uint8_t kNumPixels = 31;

enum LogicalId : uint8_t {
  D1_A = 0,
  D1_B,
  D1_C,
  D1_D,
  D1_E,
  D1_F,
  D1_G,
  D2_A,
  D2_B,
  D2_C,
  D2_D,
  D2_E,
  D2_F,
  D2_G,
  COLON_TOP,
  COLON_BOTTOM,
  D3_A,
  D3_B,
  D3_C,
  D3_D,
  D3_E,
  D3_F,
  D3_G,
  D4_A,
  D4_B,
  D4_C,
  D4_D,
  D4_E,
  D4_F,
  D4_G,
  DECIMAL,
  LOGICAL_COUNT = 31,
  LOG_UNUSED = 255
};

constexpr uint8_t kLogicalCount = static_cast<uint8_t>(LOGICAL_COUNT);

inline const char* logicalName(uint8_t logical) {
  switch (logical) {
    case D1_A: return "D1_A";
    case D1_B: return "D1_B";
    case D1_C: return "D1_C";
    case D1_D: return "D1_D";
    case D1_E: return "D1_E";
    case D1_F: return "D1_F";
    case D1_G: return "D1_G";
    case D2_A: return "D2_A";
    case D2_B: return "D2_B";
    case D2_C: return "D2_C";
    case D2_D: return "D2_D";
    case D2_E: return "D2_E";
    case D2_F: return "D2_F";
    case D2_G: return "D2_G";
    case COLON_TOP: return "COLON_TOP";
    case COLON_BOTTOM: return "COLON_BOTTOM";
    case D3_A: return "D3_A";
    case D3_B: return "D3_B";
    case D3_C: return "D3_C";
    case D3_D: return "D3_D";
    case D3_E: return "D3_E";
    case D3_F: return "D3_F";
    case D3_G: return "D3_G";
    case D4_A: return "D4_A";
    case D4_B: return "D4_B";
    case D4_C: return "D4_C";
    case D4_D: return "D4_D";
    case D4_E: return "D4_E";
    case D4_F: return "D4_F";
    case D4_G: return "D4_G";
    case DECIMAL: return "DECIMAL";
    default: return "UNUSED";
  }
}
