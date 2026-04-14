#pragma once

#include <Arduino.h>

inline uint16_t checksum16(const uint8_t* data, size_t length) {
  uint16_t sum = 0;
  for (size_t i = 0; i < length; ++i) {
    sum = static_cast<uint16_t>(sum + data[i]);
  }
  return sum;
}
