#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

#include "../model/ClockConfig.h"

class SettingsStore {
 public:
  bool begin();
  bool load(String* error = nullptr);
  bool save(String* error = nullptr) const;
  void resetToDefaults();

  ClockConfig& config();
  const ClockConfig& config() const;

  void writeConfigJson(JsonObject root) const;
  void writeMetadataJson(JsonObject root) const;
  bool updateFromJson(JsonObjectConst root, bool& restartRequired, String& error);

  String exportConfigJson() const;
  String exportMappingJson() const;
  String exportLedMapHeader() const;

 private:
  static constexpr const char* kConfigPath = "/clock-config.json";

  void buildDefaults();
  bool readConfig(JsonObjectConst root, bool& restartRequired, String& error);
  static void writeColor(JsonObject root, const RgbColor& color);
  static RgbColor readColor(JsonVariantConst value, const RgbColor& fallback);

  ClockConfig config_;
};
