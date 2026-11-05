#pragma once
#include <stdint.h>
#include "LogicalIds.h"

constexpr uint8_t kPhysToLogical[kNumPixels] = {3, 4, 2, 6, 5, 1, 0, 7, 12, 8, 9, 10, 11, 13, 14, 15, 21, 16, 17, 22, 20, 19, 18, 27, 28, 23, 24, 29, 25, 26, 30, };
constexpr uint8_t kLogicalToPhys[LOGICAL_COUNT] = { 6, 5, 2, 0, 1, 4, 3, 7, 9, 10, 11, 12, 8, 13, 14, 15, 17, 18, 22, 21, 20, 16, 19, 25, 26, 28, 29, 23, 24, 27, 30 };
