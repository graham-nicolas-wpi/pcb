#include "LedMap.h"

LedMap::LedMap() {
  loadDefaults();
  buildInverse();
}

void LedMap::loadDefaults() {
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    physToLogical[index] = index;
  }
}

void LedMap::loadFromArray(const uint8_t* values, size_t count) {
  for (size_t index = 0; index < kNumPixels; ++index) {
    physToLogical[index] = (index < count) ? values[index] : LOG_UNUSED;
  }
  buildInverse();
}

void LedMap::copyToArray(uint8_t* values, size_t count) const {
  for (size_t index = 0; index < count && index < kNumPixels; ++index) {
    values[index] = physToLogical[index];
  }
}

void LedMap::buildInverse() {
  for (uint8_t index = 0; index < LOGICAL_COUNT; ++index) {
    logicalToPhys[index] = LOG_UNUSED;
  }
  for (uint8_t physical = 0; physical < kNumPixels; ++physical) {
    const uint8_t logical = physToLogical[physical];
    if (logical < LOGICAL_COUNT) {
      logicalToPhys[logical] = physical;
    }
  }
}

bool LedMap::assign(uint8_t physical, uint8_t logical) {
  if (physical >= kNumPixels) {
    return false;
  }
  if (logical != LOG_UNUSED && logical >= LOGICAL_COUNT) {
    return false;
  }
  physToLogical[physical] = logical;
  buildInverse();
  return true;
}

bool LedMap::validateNoDuplicates() const {
  bool used[LOGICAL_COUNT] = {false};
  for (uint8_t physical = 0; physical < kNumPixels; ++physical) {
    const uint8_t logical = physToLogical[physical];
    if (logical == LOG_UNUSED) {
      continue;
    }
    if (logical >= LOGICAL_COUNT || used[logical]) {
      return false;
    }
    used[logical] = true;
  }
  return true;
}
