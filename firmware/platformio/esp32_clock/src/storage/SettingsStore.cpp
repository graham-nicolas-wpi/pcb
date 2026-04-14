#include "SettingsStore.h"

#include <LittleFS.h>

namespace {
static void copyPalette(ColorPalette& target, const ColorPalette& source) {
  target = source;
}

static PresetDefinition makePreset(const char* id, const char* name, const ColorPalette& palette,
                                   EffectMode effect, bool rainbowClock) {
  PresetDefinition preset;
  preset.id = id;
  preset.name = name;
  copyPalette(preset.palette, palette);
  preset.effect = effect;
  preset.rainbowClock = rainbowClock;
  return preset;
}

static WidgetDefinition makeWidget(const char* id, const char* name, WidgetType type,
                                   const RgbColor& primary, const RgbColor& secondary,
                                   uint16_t thresholdSeconds, uint8_t speed) {
  WidgetDefinition widget;
  widget.id = id;
  widget.name = name;
  widget.type = type;
  widget.enabled = true;
  widget.primary = primary;
  widget.secondary = secondary;
  widget.thresholdSeconds = thresholdSeconds;
  widget.speed = speed;
  return widget;
}

static FaceDefinition makeFace(const char* id, const char* name, ClockMode baseMode,
                               const char* presetId, EffectMode effect, bool showSeconds,
                               bool showColon, bool showDecimal, bool autoTimerFormat) {
  FaceDefinition face;
  face.id = id;
  face.name = name;
  face.baseMode = baseMode;
  face.presetId = presetId;
  face.effect = effect;
  face.showSeconds = showSeconds;
  face.showColon = showColon;
  face.showDecimal = showDecimal;
  face.autoTimerFormat = autoTimerFormat;
  face.widgetCount = 0;
  return face;
}

static void addWidgetToFace(FaceDefinition& face, const char* widgetId) {
  if (face.widgetCount >= ProjectConfig::kMaxFaceWidgets) {
    return;
  }
  face.widgetIds[face.widgetCount++] = widgetId;
}

static String boolString(bool value) {
  return value ? "true" : "false";
}
}

bool SettingsStore::begin() {
  if (!LittleFS.begin(true)) {
    return false;
  }
  buildDefaults();
  String error;
  load(&error);
  return true;
}

ClockConfig& SettingsStore::config() { return config_; }

const ClockConfig& SettingsStore::config() const { return config_; }

void SettingsStore::resetToDefaults() { buildDefaults(); }

void SettingsStore::buildDefaults() {
  config_ = ClockConfig();

  for (uint8_t index = 0; index < kNumPixels; ++index) {
    config_.mapping.physToLogical[index] = index;
  }

  ColorPalette warm = {
      makeRgbColor(255, 120, 40), makeRgbColor(255, 120, 40), makeRgbColor(255, 120, 40),
      makeRgbColor(255, 120, 40), makeRgbColor(0, 120, 255), makeRgbColor(255, 255, 255),
      makeRgbColor(80, 255, 120), makeRgbColor(80, 255, 120), makeRgbColor(80, 255, 120),
      makeRgbColor(255, 120, 40), makeRgbColor(255, 120, 40), makeRgbColor(255, 255, 255),
  };
  ColorPalette ice = {
      makeRgbColor(120, 220, 255), makeRgbColor(120, 220, 255), makeRgbColor(120, 220, 255),
      makeRgbColor(120, 220, 255), makeRgbColor(0, 80, 255),   makeRgbColor(220, 245, 255),
      makeRgbColor(80, 255, 220),  makeRgbColor(80, 255, 220),  makeRgbColor(80, 255, 220),
      makeRgbColor(120, 220, 255), makeRgbColor(120, 220, 255), makeRgbColor(220, 245, 255),
  };
  ColorPalette matrix = {
      makeRgbColor(0, 255, 80),   makeRgbColor(0, 255, 80),   makeRgbColor(0, 255, 80),
      makeRgbColor(0, 255, 80),   makeRgbColor(0, 120, 40),   makeRgbColor(180, 255, 180),
      makeRgbColor(80, 255, 120), makeRgbColor(80, 255, 120), makeRgbColor(80, 255, 120),
      makeRgbColor(0, 255, 80),   makeRgbColor(0, 255, 80),   makeRgbColor(180, 255, 180),
  };
  ColorPalette sunset = {
      makeRgbColor(255, 90, 40),   makeRgbColor(255, 90, 40),   makeRgbColor(255, 90, 40),
      makeRgbColor(255, 90, 40),   makeRgbColor(255, 0, 120),   makeRgbColor(255, 220, 120),
      makeRgbColor(255, 180, 80),  makeRgbColor(255, 180, 80),  makeRgbColor(255, 180, 80),
      makeRgbColor(255, 90, 40),   makeRgbColor(255, 90, 40),   makeRgbColor(255, 220, 120),
  };
  ColorPalette rainbow = {
      makeRgbColor(255, 0, 0),   makeRgbColor(255, 120, 0), makeRgbColor(0, 255, 0),
      makeRgbColor(0, 120, 255), makeRgbColor(180, 0, 255), makeRgbColor(0, 120, 255),
      makeRgbColor(180, 0, 255), makeRgbColor(180, 0, 255), makeRgbColor(180, 0, 255),
      makeRgbColor(255, 0, 0),   makeRgbColor(255, 120, 0), makeRgbColor(0, 120, 255),
  };
  ColorPalette party = {
      makeRgbColor(255, 0, 255), makeRgbColor(255, 0, 255), makeRgbColor(255, 0, 255),
      makeRgbColor(255, 0, 255), makeRgbColor(0, 255, 255), makeRgbColor(255, 255, 0),
      makeRgbColor(255, 80, 80), makeRgbColor(255, 80, 80), makeRgbColor(255, 80, 80),
      makeRgbColor(255, 0, 255), makeRgbColor(255, 0, 255), makeRgbColor(255, 255, 0),
  };

  config_.presetCount = 6;
  config_.presets[0] = makePreset("warm-classic", "Warm Classic", warm, EffectMode::NONE, false);
  config_.presets[1] = makePreset("ice-blue", "Ice Blue", ice, EffectMode::NONE, false);
  config_.presets[2] = makePreset("matrix", "Matrix", matrix, EffectMode::NONE, false);
  config_.presets[3] = makePreset("sunset", "Sunset", sunset, EffectMode::PULSE, false);
  config_.presets[4] = makePreset("rainbow", "Rainbow", rainbow, EffectMode::RAINBOW, true);
  config_.presets[5] = makePreset("party", "Party", party, EffectMode::PARTY, false);

  config_.widgetCount = 5;
  config_.widgets[0] = makeWidget("pulse-colon", "Pulse Colon", WidgetType::PULSE_COLON,
                                  makeRgbColor(255, 255, 255), makeRgbColor(0, 120, 255), 30,
                                  160);
  config_.widgets[1] = makeWidget("rainbow-digits", "Rainbow Digits",
                                  WidgetType::RAINBOW_DIGITS, makeRgbColor(255, 0, 0),
                                  makeRgbColor(0, 120, 255), 0, 180);
  config_.widgets[2] = makeWidget("timer-warning", "Timer Warning",
                                  WidgetType::TIMER_WARNING, makeRgbColor(255, 40, 40),
                                  makeRgbColor(255, 180, 0), 30, 220);
  config_.widgets[3] = makeWidget("seconds-spark", "Seconds Spark",
                                  WidgetType::SECONDS_SPARK, makeRgbColor(255, 255, 255),
                                  makeRgbColor(0, 180, 255), 0, 140);
  config_.widgets[4] = makeWidget("accent-sweep", "Accent Sweep",
                                  WidgetType::ACCENT_SWEEP, makeRgbColor(0, 180, 255),
                                  makeRgbColor(180, 0, 255), 0, 200);

  config_.faceCount = 5;
  config_.faces[0] = makeFace("classic-clock", "Classic Clock", ClockMode::CLOCK,
                              "warm-classic", EffectMode::NONE, false, true, false, true);
  addWidgetToFace(config_.faces[0], "pulse-colon");
  config_.faces[1] = makeFace("clock-seconds", "Clock + Seconds Widget",
                              ClockMode::CLOCK_SECONDS, "ice-blue", EffectMode::NONE, true,
                              true, false, true);
  addWidgetToFace(config_.faces[1], "seconds-spark");
  addWidgetToFace(config_.faces[1], "pulse-colon");
  config_.faces[2] = makeFace("timer-focus", "Timer Focus", ClockMode::TIMER, "sunset",
                              EffectMode::PULSE, true, true, false, true);
  addWidgetToFace(config_.faces[2], "timer-warning");
  config_.faces[3] = makeFace("stopwatch-sport", "Stopwatch Sport", ClockMode::STOPWATCH,
                              "matrix", EffectMode::NONE, true, true, false, true);
  addWidgetToFace(config_.faces[3], "accent-sweep");
  config_.faces[4] = makeFace("rainbow-demo", "Rainbow Demo", ClockMode::COLOR_DEMO,
                              "rainbow", EffectMode::RAINBOW, true, true, true, true);
  addWidgetToFace(config_.faces[4], "rainbow-digits");
  addWidgetToFace(config_.faces[4], "pulse-colon");

  config_.runtime.activePresetId = "warm-classic";
  config_.runtime.activeFaceId = "classic-clock";
}

bool SettingsStore::load(String* error) {
  if (!LittleFS.exists(kConfigPath)) {
    return save(error);
  }

  File file = LittleFS.open(kConfigPath, "r");
  if (!file) {
    if (error != nullptr) *error = "Unable to open config file";
    return false;
  }

  DynamicJsonDocument doc(32768);
  DeserializationError jsonError = deserializeJson(doc, file);
  file.close();
  if (jsonError) {
    if (error != nullptr) *error = jsonError.c_str();
    return false;
  }

  bool restartRequired = false;
  String parseError;
  if (!readConfig(doc.as<JsonObjectConst>(), restartRequired, parseError)) {
    if (error != nullptr) *error = parseError;
    return false;
  }
  return true;
}

bool SettingsStore::save(String* error) const {
  File file = LittleFS.open(kConfigPath, "w");
  if (!file) {
    if (error != nullptr) *error = "Unable to write config file";
    return false;
  }

  DynamicJsonDocument doc(32768);
  JsonObject root = doc.to<JsonObject>();
  writeConfigJson(root);
  if (serializeJsonPretty(doc, file) == 0) {
    if (error != nullptr) *error = "serializeJsonPretty wrote zero bytes";
    file.close();
    return false;
  }
  file.close();
  return true;
}

void SettingsStore::writeColor(JsonObject root, const RgbColor& color) {
  root["r"] = color.r;
  root["g"] = color.g;
  root["b"] = color.b;
}

RgbColor SettingsStore::readColor(JsonVariantConst value, const RgbColor& fallback) {
  if (!value.is<JsonObjectConst>()) {
    return fallback;
  }
  JsonObjectConst object = value.as<JsonObjectConst>();
  return makeRgbColor(object["r"] | fallback.r, object["g"] | fallback.g, object["b"] | fallback.b);
}

void SettingsStore::writeConfigJson(JsonObject root) const {
  JsonObject network = root.createNestedObject("network");
  network["apSsid"] = config_.network.apSsid;
  network["apPassword"] = config_.network.apPassword;
  network["staEnabled"] = config_.network.staEnabled;
  network["staSsid"] = config_.network.staSsid;
  network["staPassword"] = config_.network.staPassword;
  network["hostname"] = config_.network.hostname;

  JsonObject rendererLink = root.createNestedObject("rendererLink");
  rendererLink["baud"] = config_.rendererLink.baud;
  rendererLink["txPin"] = config_.rendererLink.txPin;
  rendererLink["rxPin"] = config_.rendererLink.rxPin;
  rendererLink["buttonEvents"] = config_.rendererLink.buttonEvents;

  JsonObject time = root.createNestedObject("time");
  time["ntpEnabled"] = config_.time.ntpEnabled;
  time["timezone"] = config_.time.timezone;
  time["ntpPrimary"] = config_.time.ntpPrimary;
  time["ntpSecondary"] = config_.time.ntpSecondary;
  time["rtcSyncIntervalSeconds"] = config_.time.rtcSyncIntervalSeconds;

  JsonObject runtime = root.createNestedObject("runtime");
  runtime["mode"] = clockModeName(config_.runtime.mode);
  runtime["hourMode"] = hourModeName(config_.runtime.hourMode);
  runtime["secondsMode"] = secondsModeName(config_.runtime.secondsMode);
  runtime["timerFormat"] = timerFormatName(config_.runtime.timerFormat);
  runtime["effect"] = effectModeName(config_.runtime.effect);
  runtime["brightness"] = config_.runtime.brightness;
  runtime["colonBrightness"] = config_.runtime.colonBrightness;
  runtime["rainbowEnabled"] = config_.runtime.rainbowEnabled;
  runtime["timerPresetSeconds"] = config_.runtime.timerPresetSeconds;
  runtime["activePresetId"] = config_.runtime.activePresetId;
  runtime["activeFaceId"] = config_.runtime.activeFaceId;

  JsonArray presets = root.createNestedArray("presets");
  for (size_t index = 0; index < config_.presetCount; ++index) {
    const PresetDefinition& preset = config_.presets[index];
    JsonObject presetObject = presets.createNestedObject();
    presetObject["id"] = preset.id;
    presetObject["name"] = preset.name;
    presetObject["effect"] = effectModeName(preset.effect);
    presetObject["rainbowClock"] = preset.rainbowClock;
    JsonObject palette = presetObject.createNestedObject("palette");
    writeColor(palette.createNestedObject("digit1"), preset.palette.digit1);
    writeColor(palette.createNestedObject("digit2"), preset.palette.digit2);
    writeColor(palette.createNestedObject("digit3"), preset.palette.digit3);
    writeColor(palette.createNestedObject("digit4"), preset.palette.digit4);
    writeColor(palette.createNestedObject("accent"), preset.palette.accent);
    writeColor(palette.createNestedObject("seconds"), preset.palette.seconds);
    writeColor(palette.createNestedObject("timerHours"), preset.palette.timerHours);
    writeColor(palette.createNestedObject("timerMinutes"), preset.palette.timerMinutes);
    writeColor(palette.createNestedObject("timerSeconds"), preset.palette.timerSeconds);
    writeColor(palette.createNestedObject("stopwatchHours"), preset.palette.stopwatchHours);
    writeColor(palette.createNestedObject("stopwatchMinutes"), preset.palette.stopwatchMinutes);
    writeColor(palette.createNestedObject("stopwatchSeconds"), preset.palette.stopwatchSeconds);
  }

  JsonArray widgets = root.createNestedArray("widgets");
  for (size_t index = 0; index < config_.widgetCount; ++index) {
    const WidgetDefinition& widget = config_.widgets[index];
    JsonObject widgetObject = widgets.createNestedObject();
    widgetObject["id"] = widget.id;
    widgetObject["name"] = widget.name;
    widgetObject["type"] = widgetTypeName(widget.type);
    widgetObject["enabled"] = widget.enabled;
    widgetObject["thresholdSeconds"] = widget.thresholdSeconds;
    widgetObject["speed"] = widget.speed;
    writeColor(widgetObject.createNestedObject("primary"), widget.primary);
    writeColor(widgetObject.createNestedObject("secondary"), widget.secondary);
  }

  JsonArray faces = root.createNestedArray("faces");
  for (size_t index = 0; index < config_.faceCount; ++index) {
    const FaceDefinition& face = config_.faces[index];
    JsonObject faceObject = faces.createNestedObject();
    faceObject["id"] = face.id;
    faceObject["name"] = face.name;
    faceObject["baseMode"] = clockModeName(face.baseMode);
    faceObject["presetId"] = face.presetId;
    faceObject["effect"] = effectModeName(face.effect);
    faceObject["showSeconds"] = face.showSeconds;
    faceObject["showColon"] = face.showColon;
    faceObject["showDecimal"] = face.showDecimal;
    faceObject["autoTimerFormat"] = face.autoTimerFormat;
    JsonArray widgetIds = faceObject.createNestedArray("widgetIds");
    for (uint8_t widgetIndex = 0; widgetIndex < face.widgetCount; ++widgetIndex) {
      widgetIds.add(face.widgetIds[widgetIndex]);
    }
  }
}

void SettingsStore::writeMetadataJson(JsonObject root) const {
  JsonArray modes = root.createNestedArray("modes");
  modes.add("CLOCK");
  modes.add("CLOCK_SECONDS");
  modes.add("TIMER");
  modes.add("STOPWATCH");
  modes.add("COLOR_DEMO");

  JsonArray hourModes = root.createNestedArray("hourModes");
  hourModes.add("24H");
  hourModes.add("12H");

  JsonObject rendererLink = root.createNestedObject("rendererLink");
  rendererLink["defaultBaud"] = ProjectConfig::kDefaultRendererBaud;
  rendererLink["defaultTxPin"] = ProjectConfig::kDefaultRendererTxPin;
  rendererLink["defaultRxPin"] = ProjectConfig::kDefaultRendererRxPin;

  JsonArray secondsModes = root.createNestedArray("secondsModes");
  secondsModes.add("BLINK");
  secondsModes.add("ON");
  secondsModes.add("OFF");

  JsonArray timerFormats = root.createNestedArray("timerFormats");
  timerFormats.add("AUTO");
  timerFormats.add("HHMM");
  timerFormats.add("MMSS");
  timerFormats.add("SSCS");

  JsonArray effects = root.createNestedArray("effects");
  effects.add("NONE");
  effects.add("RAINBOW");
  effects.add("PULSE");
  effects.add("PARTY");

  JsonArray widgetTypes = root.createNestedArray("widgetTypes");
  widgetTypes.add("PULSE_COLON");
  widgetTypes.add("RAINBOW_DIGITS");
  widgetTypes.add("TIMER_WARNING");
  widgetTypes.add("SECONDS_SPARK");
  widgetTypes.add("ACCENT_SWEEP");

  JsonArray logicalTargets = root.createNestedArray("logicalTargets");
  for (uint8_t index = 0; index < kLogicalCount; ++index) {
    JsonObject target = logicalTargets.createNestedObject();
    target["id"] = index;
    target["name"] = logicalName(index);
  }
}

bool SettingsStore::readConfig(JsonObjectConst root, bool& restartRequired, String& error) {
  ClockConfig original = config_;
  ClockConfig updated = config_;

  JsonObjectConst hardware = root["hardware"];
  if (!hardware.isNull()) {
    updated.hardware.pixelCount = hardware["pixelCount"] | updated.hardware.pixelCount;
    updated.hardware.pixelPin = hardware["pixelPin"] | updated.hardware.pixelPin;
    updated.hardware.rtcSda = hardware["rtcSda"] | updated.hardware.rtcSda;
    updated.hardware.rtcScl = hardware["rtcScl"] | updated.hardware.rtcScl;
    updated.hardware.colorOrder = String(static_cast<const char*>(hardware["colorOrder"] | updated.hardware.colorOrder.c_str()));
    JsonArrayConst buttonPins = hardware["buttonPins"].as<JsonArrayConst>();
    if (!buttonPins.isNull() && buttonPins.size() == 3) {
      for (uint8_t index = 0; index < 3; ++index) {
        updated.hardware.buttonPins[index] = buttonPins[index] | updated.hardware.buttonPins[index];
      }
    }
  }

  JsonObjectConst network = root["network"];
  if (!network.isNull()) {
    updated.network.apSsid = String(static_cast<const char*>(network["apSsid"] | updated.network.apSsid.c_str()));
    updated.network.apPassword = String(static_cast<const char*>(network["apPassword"] | updated.network.apPassword.c_str()));
    updated.network.staEnabled = network["staEnabled"] | updated.network.staEnabled;
    updated.network.staSsid = String(static_cast<const char*>(network["staSsid"] | updated.network.staSsid.c_str()));
    updated.network.staPassword = String(static_cast<const char*>(network["staPassword"] | updated.network.staPassword.c_str()));
    updated.network.hostname = String(static_cast<const char*>(network["hostname"] | updated.network.hostname.c_str()));
  }

  JsonObjectConst rendererLink = root["rendererLink"];
  if (!rendererLink.isNull()) {
    updated.rendererLink.baud = rendererLink["baud"] | updated.rendererLink.baud;
    updated.rendererLink.txPin = rendererLink["txPin"] | updated.rendererLink.txPin;
    updated.rendererLink.rxPin = rendererLink["rxPin"] | updated.rendererLink.rxPin;
    updated.rendererLink.buttonEvents =
        rendererLink["buttonEvents"] | updated.rendererLink.buttonEvents;
  }

  JsonObjectConst time = root["time"];
  if (!time.isNull()) {
    updated.time.ntpEnabled = time["ntpEnabled"] | updated.time.ntpEnabled;
    updated.time.timezone =
        String(static_cast<const char*>(time["timezone"] | updated.time.timezone.c_str()));
    updated.time.ntpPrimary =
        String(static_cast<const char*>(time["ntpPrimary"] | updated.time.ntpPrimary.c_str()));
    updated.time.ntpSecondary = String(
        static_cast<const char*>(time["ntpSecondary"] | updated.time.ntpSecondary.c_str()));
    updated.time.rtcSyncIntervalSeconds =
        time["rtcSyncIntervalSeconds"] | updated.time.rtcSyncIntervalSeconds;
  }

  JsonObjectConst runtime = root["runtime"];
  if (!runtime.isNull()) {
    updated.runtime.mode = clockModeFromString(String(static_cast<const char*>(runtime["mode"] | clockModeName(updated.runtime.mode))));
    updated.runtime.hourMode = hourModeFromString(String(static_cast<const char*>(runtime["hourMode"] | hourModeName(updated.runtime.hourMode))));
    updated.runtime.secondsMode = secondsModeFromString(String(static_cast<const char*>(runtime["secondsMode"] | secondsModeName(updated.runtime.secondsMode))));
    updated.runtime.timerFormat = timerFormatFromString(String(static_cast<const char*>(runtime["timerFormat"] | timerFormatName(updated.runtime.timerFormat))));
    updated.runtime.effect = effectModeFromString(String(static_cast<const char*>(runtime["effect"] | effectModeName(updated.runtime.effect))));
    updated.runtime.brightness = runtime["brightness"] | updated.runtime.brightness;
    updated.runtime.colonBrightness = runtime["colonBrightness"] | updated.runtime.colonBrightness;
    updated.runtime.rainbowEnabled = runtime["rainbowEnabled"] | updated.runtime.rainbowEnabled;
    updated.runtime.calibrationMode = runtime["calibrationMode"] | updated.runtime.calibrationMode;
    updated.runtime.calibrationCursor = runtime["calibrationCursor"] | updated.runtime.calibrationCursor;
    updated.runtime.timerPresetSeconds = runtime["timerPresetSeconds"] | updated.runtime.timerPresetSeconds;
    updated.runtime.activePresetId = String(static_cast<const char*>(runtime["activePresetId"] | updated.runtime.activePresetId.c_str()));
    updated.runtime.activeFaceId = String(static_cast<const char*>(runtime["activeFaceId"] | updated.runtime.activeFaceId.c_str()));
  }

  JsonArrayConst mapping = root["mapping"].as<JsonArrayConst>();
  if (!mapping.isNull()) {
    if (mapping.size() != kNumPixels) {
      error = "mapping must contain 31 entries";
      return false;
    }
    for (uint8_t index = 0; index < kNumPixels; ++index) {
      updated.mapping.physToLogical[index] = mapping[index] | updated.mapping.physToLogical[index];
    }
  }

  JsonArrayConst presets = root["presets"].as<JsonArrayConst>();
  if (!presets.isNull()) {
    updated.presetCount = min(static_cast<size_t>(presets.size()), ProjectConfig::kMaxPresets);
    for (size_t index = 0; index < updated.presetCount; ++index) {
      JsonObjectConst presetObject = presets[index].as<JsonObjectConst>();
      PresetDefinition preset;
      preset.id = String(static_cast<const char*>(presetObject["id"] | ""));
      preset.name = String(static_cast<const char*>(presetObject["name"] | ""));
      preset.effect = effectModeFromString(String(static_cast<const char*>(presetObject["effect"] | "NONE")));
      preset.rainbowClock = presetObject["rainbowClock"] | false;
      JsonObjectConst palette = presetObject["palette"].as<JsonObjectConst>();
      preset.palette.digit1 = readColor(palette["digit1"], makeRgbColor(255, 255, 255));
      preset.palette.digit2 = readColor(palette["digit2"], preset.palette.digit1);
      preset.palette.digit3 = readColor(palette["digit3"], preset.palette.digit1);
      preset.palette.digit4 = readColor(palette["digit4"], preset.palette.digit1);
      preset.palette.accent = readColor(palette["accent"], makeRgbColor(0, 120, 255));
      preset.palette.seconds = readColor(palette["seconds"], makeRgbColor(255, 255, 255));
      preset.palette.timerHours = readColor(palette["timerHours"], makeRgbColor(80, 255, 120));
      preset.palette.timerMinutes = readColor(palette["timerMinutes"], preset.palette.timerHours);
      preset.palette.timerSeconds = readColor(palette["timerSeconds"], preset.palette.timerHours);
      preset.palette.stopwatchHours = readColor(palette["stopwatchHours"], preset.palette.digit1);
      preset.palette.stopwatchMinutes = readColor(palette["stopwatchMinutes"], preset.palette.digit2);
      preset.palette.stopwatchSeconds = readColor(palette["stopwatchSeconds"], preset.palette.seconds);
      updated.presets[index] = preset;
    }
  }

  JsonArrayConst widgets = root["widgets"].as<JsonArrayConst>();
  if (!widgets.isNull()) {
    updated.widgetCount = min(static_cast<size_t>(widgets.size()), ProjectConfig::kMaxWidgets);
    for (size_t index = 0; index < updated.widgetCount; ++index) {
      JsonObjectConst widgetObject = widgets[index].as<JsonObjectConst>();
      WidgetDefinition widget;
      widget.id = String(static_cast<const char*>(widgetObject["id"] | ""));
      widget.name = String(static_cast<const char*>(widgetObject["name"] | ""));
      widget.type = widgetTypeFromString(String(static_cast<const char*>(widgetObject["type"] | "PULSE_COLON")));
      widget.enabled = widgetObject["enabled"] | true;
      widget.thresholdSeconds = widgetObject["thresholdSeconds"] | 30;
      widget.speed = widgetObject["speed"] | 128;
      widget.primary = readColor(widgetObject["primary"], makeRgbColor(255, 255, 255));
      widget.secondary = readColor(widgetObject["secondary"], makeRgbColor(0, 120, 255));
      updated.widgets[index] = widget;
    }
  }

  JsonArrayConst faces = root["faces"].as<JsonArrayConst>();
  if (!faces.isNull()) {
    updated.faceCount = min(static_cast<size_t>(faces.size()), ProjectConfig::kMaxFaces);
    for (size_t index = 0; index < updated.faceCount; ++index) {
      JsonObjectConst faceObject = faces[index].as<JsonObjectConst>();
      FaceDefinition face;
      face.id = String(static_cast<const char*>(faceObject["id"] | ""));
      face.name = String(static_cast<const char*>(faceObject["name"] | ""));
      face.baseMode = clockModeFromString(String(static_cast<const char*>(faceObject["baseMode"] | "CLOCK")));
      face.presetId = String(static_cast<const char*>(faceObject["presetId"] | ""));
      face.effect = effectModeFromString(String(static_cast<const char*>(faceObject["effect"] | "NONE")));
      face.showSeconds = faceObject["showSeconds"] | false;
      face.showColon = faceObject["showColon"] | true;
      face.showDecimal = faceObject["showDecimal"] | false;
      face.autoTimerFormat = faceObject["autoTimerFormat"] | true;
      face.widgetCount = 0;
      JsonArrayConst widgetIds = faceObject["widgetIds"].as<JsonArrayConst>();
      for (JsonVariantConst widgetId : widgetIds) {
        if (face.widgetCount >= ProjectConfig::kMaxFaceWidgets) {
          break;
        }
        face.widgetIds[face.widgetCount++] = String(static_cast<const char*>(widgetId | ""));
      }
      updated.faces[index] = face;
    }
  }

  config_ = updated;
  restartRequired = (original.rendererLink.baud != config_.rendererLink.baud) ||
                    (original.rendererLink.txPin != config_.rendererLink.txPin) ||
                    (original.rendererLink.rxPin != config_.rendererLink.rxPin) ||
                    (original.network.apSsid != config_.network.apSsid) ||
                    (original.network.apPassword != config_.network.apPassword) ||
                    (original.network.staEnabled != config_.network.staEnabled) ||
                    (original.network.staSsid != config_.network.staSsid) ||
                    (original.network.staPassword != config_.network.staPassword) ||
                    (original.network.hostname != config_.network.hostname);
  return true;
}

bool SettingsStore::updateFromJson(JsonObjectConst root, bool& restartRequired, String& error) {
  if (!readConfig(root, restartRequired, error)) {
    return false;
  }
  return save(&error);
}

String SettingsStore::exportConfigJson() const {
  DynamicJsonDocument doc(32768);
  JsonObject root = doc.to<JsonObject>();
  writeConfigJson(root);
  String output;
  serializeJsonPretty(doc, output);
  return output;
}
