#include "LedMap.h"

LedMap::LedMap() {
  loadDefaults();
  buildInverse();
}

void LedMap::loadDefaults() {
  for (uint8_t i = 0; i < kNumPixels; ++i) physToLogical[i] = kPhysToLogical[i];
}

uint16_t LedMap::checksum16(const uint8_t* data, size_t n) const {
  uint16_t s = 0;
  for (size_t i = 0; i < n; ++i) s = static_cast<uint16_t>(s + data[i]);
  return s;
}

void LedMap::buildInverse() {
  for (uint8_t i = 0; i < LOGICAL_COUNT; ++i) logicalToPhys[i] = LOG_UNUSED;
  for (uint8_t p = 0; p < kNumPixels; ++p) {
    uint8_t l = physToLogical[p];
    if (l < LOGICAL_COUNT) logicalToPhys[l] = p;
  }
}

bool LedMap::validateNoDuplicates() const {
  bool used[LOGICAL_COUNT] = {false};
  for (uint8_t p = 0; p < kNumPixels; ++p) {
    uint8_t l = physToLogical[p];
    if (l == LOG_UNUSED) continue;
    if (l >= LOGICAL_COUNT) return false;
    if (used[l]) return false;
    used[l] = true;
  }
  return true;
}

bool LedMap::saveToEeprom() const {
  if (!validateNoDuplicates()) return false;
  Blob blob{};
  blob.magic = kMagic;
  blob.version = kVersion;
  blob.numPixels = kNumPixels;
  for (uint8_t i = 0; i < kNumPixels; ++i) blob.physToLogical[i] = physToLogical[i];
  blob.checksum = checksum16(blob.physToLogical, kNumPixels);
  EEPROM.put(kEepromAddr, blob);
  return true;
}

bool LedMap::loadFromEeprom() {
  Blob blob{};
  EEPROM.get(kEepromAddr, blob);
  if (blob.magic != kMagic || blob.version != kVersion || blob.numPixels != kNumPixels) return false;
  if (blob.checksum != checksum16(blob.physToLogical, kNumPixels)) return false;
  for (uint8_t i = 0; i < kNumPixels; ++i) physToLogical[i] = blob.physToLogical[i];
  buildInverse();
  return validateNoDuplicates();
}

void LedMap::loadOrDefault() {
  if (!loadFromEeprom()) {
    loadDefaults();
    buildInverse();
  }
}
