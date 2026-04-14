#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "LogicalIds.h"
#include "Generated_LedMap.h"

class LedMap {
public:
  struct Blob {
    uint32_t magic;
    uint8_t version;
    uint8_t numPixels;
    uint8_t physToLogical[kNumPixels];
    uint16_t checksum;
  };

  static constexpr uint16_t kEepromAddr = 0;
  static constexpr uint32_t kMagic = 0x4D415031UL; // MAP1
  static constexpr uint8_t kVersion = 1;

  LedMap();
  void loadDefaults();
  bool loadFromEeprom();
  bool saveToEeprom() const;
  void loadOrDefault();
  void buildInverse();
  bool validateNoDuplicates() const;
  bool assign(uint8_t physical, uint8_t logical);
  uint16_t checksum16(const uint8_t* data, size_t n) const;

  uint8_t physToLogical[kNumPixels];
  uint8_t logicalToPhys[LOGICAL_COUNT];
};
