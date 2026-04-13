#pragma once
#include <stdint.h>

constexpr uint8_t kNumPixels = 31;

enum LogicalId : uint8_t {
  D1_A = 0, D1_B, D1_C, D1_D, D1_E, D1_F, D1_G,
  D2_A, D2_B, D2_C, D2_D, D2_E, D2_F, D2_G,
  COLON_TOP, COLON_BOTTOM,
  D3_A, D3_B, D3_C, D3_D, D3_E, D3_F, D3_G,
  D4_A, D4_B, D4_C, D4_D, D4_E, D4_F, D4_G,
  DECIMAL,
  LOGICAL_COUNT = 31,
  LOG_UNUSED = 255
};
