#pragma once

#include <Arduino.h>

#include "LogicalIds.h"

class LedMap {
 public:
  LedMap();

  void loadDefaults();
  void loadFromArray(const uint8_t* values, size_t count);
  void copyToArray(uint8_t* values, size_t count) const;
  void buildInverse();
  bool validateNoDuplicates() const;
  bool assign(uint8_t physical, uint8_t logical);

  uint8_t physToLogical[kNumPixels];
  uint8_t logicalToPhys[LOGICAL_COUNT];
};
