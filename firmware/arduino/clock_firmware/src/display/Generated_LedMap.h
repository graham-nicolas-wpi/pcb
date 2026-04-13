#pragma once
#include <stdint.h>
#include "LogicalIds.h"

constexpr uint8_t kPhysToLogical[kNumPixels] = {3, 4, 2, 6, 5, 1, 0, 7, 12, 8, 9, 10, 11, 13, 14, 15, 21, 16, 17, 22, 20, 19, 18, 27, 28, 23, 24, 29, 25, 26, 30, };
constexpr uint8_t kLogicalToPhys[LOGICAL_COUNT] = { 5, 4, 1, 30, 0, 3, 2, 6, 8, 9, 10, 11, 7, 12, 13, 14, 16, 17, 21, 20, 19, 15, 18, 24, 25, 27, 28, 22, 23, 26, 29 };
