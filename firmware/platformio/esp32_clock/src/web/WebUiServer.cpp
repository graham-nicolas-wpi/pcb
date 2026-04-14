#include "WebUiServer.h"

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>

namespace {
static void fillStatusEnvelope(DynamicJsonDocument& doc, SettingsStore& store,
                               ClockRuntime& runtime, bool ok, const String& message) {
  JsonObject root = doc.to<JsonObject>();
  root["ok"] = ok;
  root["message"] = message;
  runtime.writeStatusJson(root.createNestedObject("status"), millis());
  runtime.writeRtcJson(root.createNestedObject("rtc"), millis());
  root["activePresetId"] = store.config().runtime.activePresetId;
  root["activeFaceId"] = store.config().runtime.activeFaceId;
}
}

WebUiServer::WebUiServer(SettingsStore& store, ClockRuntime& runtime)
    : store_(store), runtime_(runtime), server_(80) {}

void WebUiServer::begin() {
  configureWiFi();
  registerRoutes();
  server_.begin();
}

void WebUiServer::update() {
  server_.handleClient();
}

void WebUiServer::configureWiFi() {
  const NetworkSettings& network = store_.config().network;
  WiFi.mode(network.staEnabled ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(network.apSsid.c_str(), network.apPassword.c_str());

  if (network.staEnabled && network.staSsid.length() > 0) {
    WiFi.begin(network.staSsid.c_str(), network.staPassword.c_str());
  }

  if (network.hostname.length() > 0) {
    MDNS.begin(network.hostname.c_str());
  }
}

void WebUiServer::registerRoutes() {
  server_.on("/api/bootstrap", HTTP_GET, [this]() { handleBootstrap(); });
  server_.on("/api/config", HTTP_POST, [this]() { handleConfigSave(); });
  server_.on("/api/action", HTTP_POST, [this]() { handleAction(); });
  server_.on("/api/export/config.json", HTTP_GET, [this]() { handleExportConfig(); });
  server_.on("/api/export/mapping.json", HTTP_GET, [this]() { handleExportMapping(); });
  server_.on("/api/export/ledmap.h", HTTP_GET, [this]() { handleExportHeader(); });
  server_.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
}

void WebUiServer::appendBootstrapJson(JsonObject root) {
  runtime_.writeInfoJson(root.createNestedObject("info"));
  runtime_.writeStatusJson(root.createNestedObject("status"), millis());
  runtime_.writeRtcJson(root.createNestedObject("rtc"), millis());
  store_.writeConfigJson(root.createNestedObject("config"));
  store_.writeMetadataJson(root.createNestedObject("metadata"));

  JsonObject network = root.createNestedObject("network");
  network["apIp"] = WiFi.softAPIP().toString();
  network["staConnected"] = WiFi.status() == WL_CONNECTED;
  network["staIp"] = WiFi.localIP().toString();
  network["hostname"] = store_.config().network.hostname;
}

void WebUiServer::handleBootstrap() {
  DynamicJsonDocument doc(36864);
  JsonObject root = doc.to<JsonObject>();
  root["ok"] = true;
  appendBootstrapJson(root);
  sendJsonDocument(doc);
}

bool WebUiServer::readJsonBody(DynamicJsonDocument& doc) {
  if (!server_.hasArg("plain")) {
    return false;
  }
  const String body = server_.arg("plain");
  return !deserializeJson(doc, body);
}

void WebUiServer::sendJsonDocument(const DynamicJsonDocument& doc, int statusCode) {
  String body;
  serializeJson(doc, body);
  server_.send(statusCode, "application/json", body);
}

void WebUiServer::handleConfigSave() {
  DynamicJsonDocument doc(36864);
  if (!readJsonBody(doc)) {
    DynamicJsonDocument response(4096);
    fillStatusEnvelope(response, store_, runtime_, false, "Invalid JSON");
    sendJsonDocument(response, 400);
    return;
  }

  bool restartRequired = false;
  String error;
  if (!store_.updateFromJson(doc.as<JsonObjectConst>(), restartRequired, error)) {
    DynamicJsonDocument response(4096);
    fillStatusEnvelope(response, store_, runtime_, false, error);
    sendJsonDocument(response, 400);
    return;
  }

  runtime_.reloadFromConfig();
  DynamicJsonDocument response(4096);
  fillStatusEnvelope(response, store_, runtime_, true, "Configuration saved");
  response["restartRequired"] = restartRequired;
  sendJsonDocument(response);
}

void WebUiServer::handleAction() {
  DynamicJsonDocument doc(8192);
  if (!readJsonBody(doc)) {
    DynamicJsonDocument response(4096);
    fillStatusEnvelope(response, store_, runtime_, false, "Invalid JSON");
    sendJsonDocument(response, 400);
    return;
  }

  JsonObjectConst root = doc.as<JsonObjectConst>();
  const String action = String(static_cast<const char*>(root["action"] | ""));
  bool ok = true;
  String message = action;

  if (action == "set_mode") {
    runtime_.setMode(clockModeFromString(String(static_cast<const char*>(root["mode"] | "CLOCK"))));
  } else if (action == "set_hour_mode") {
    runtime_.setHourMode(hourModeFromString(String(static_cast<const char*>(root["hourMode"] | "24H"))));
  } else if (action == "set_seconds_mode") {
    runtime_.setSecondsMode(secondsModeFromString(String(static_cast<const char*>(root["secondsMode"] | "BLINK"))));
  } else if (action == "set_timer_format") {
    runtime_.setTimerFormat(timerFormatFromString(String(static_cast<const char*>(root["timerFormat"] | "AUTO"))));
  } else if (action == "set_effect") {
    runtime_.setEffect(effectModeFromString(String(static_cast<const char*>(root["effect"] | "NONE"))));
  } else if (action == "set_brightness") {
    runtime_.setBrightness(static_cast<uint8_t>(root["brightness"] | store_.config().runtime.brightness));
  } else if (action == "set_colon_brightness") {
    runtime_.setColonBrightness(static_cast<uint8_t>(root["colonBrightness"] | store_.config().runtime.colonBrightness));
  } else if (action == "apply_preset") {
    ok = runtime_.setActivePreset(String(static_cast<const char*>(root["presetId"] | "")));
  } else if (action == "apply_face") {
    ok = runtime_.setActiveFace(String(static_cast<const char*>(root["faceId"] | "")));
  } else if (action == "set_color") {
    JsonObjectConst color = root["color"].as<JsonObjectConst>();
    ok = runtime_.setColorTarget(
        String(static_cast<const char*>(root["target"] | "")),
        makeRgbColor(color["r"] | 0, color["g"] | 0, color["b"] | 0));
  } else if (action == "timer_set") {
    runtime_.setTimerPresetSeconds(root["seconds"] | store_.config().runtime.timerPresetSeconds);
  } else if (action == "timer_start") {
    runtime_.startTimer(millis());
  } else if (action == "timer_stop") {
    runtime_.stopTimer(millis());
  } else if (action == "timer_reset") {
    runtime_.resetTimer();
  } else if (action == "stopwatch_start") {
    runtime_.startStopwatch(millis());
  } else if (action == "stopwatch_stop") {
    runtime_.stopStopwatch(millis());
  } else if (action == "stopwatch_reset") {
    runtime_.resetStopwatch(millis());
  } else if (action == "rtc_set") {
    ok = runtime_.setRtc(DateTime(root["year"] | 2026, root["month"] | 1, root["day"] | 1,
                                  root["hour"] | 0, root["minute"] | 0, root["second"] | 0));
  } else if (action == "calibration_mode") {
    runtime_.setCalibrationMode(root["enabled"] | false);
  } else if (action == "calibration_set") {
    ok = runtime_.setCalibrationCursor(root["physical"] | 0);
  } else if (action == "calibration_next") {
    runtime_.nextCalibration(1);
  } else if (action == "calibration_prev") {
    runtime_.nextCalibration(-1);
  } else if (action == "assign") {
    ok = runtime_.assignLogical(root["logical"] | 0);
  } else if (action == "unassign") {
    runtime_.unassignCurrent();
  } else if (action == "save") {
    String error;
    ok = runtime_.saveConfigNow(&error);
    if (!ok) message = error;
  } else if (action == "load") {
    String error;
    ok = runtime_.loadConfigNow(&error);
    if (!ok) message = error;
  } else if (action == "test_segments") {
    runtime_.testSegments();
  } else if (action == "test_digits") {
    runtime_.testDigits();
  } else if (action == "test_all") {
    runtime_.testAll();
  } else {
    ok = false;
    message = "Unknown action";
  }

  DynamicJsonDocument response(4096);
  fillStatusEnvelope(response, store_, runtime_, ok, message);
  sendJsonDocument(response, ok ? 200 : 400);
}

void WebUiServer::handleExportConfig() {
  server_.sendHeader("Content-Disposition", "attachment; filename=\"clock-config.json\"");
  server_.send(200, "application/json", store_.exportConfigJson());
}

void WebUiServer::handleExportMapping() {
  server_.sendHeader("Content-Disposition", "attachment; filename=\"clock-map.json\"");
  server_.send(200, "application/json", store_.exportMappingJson());
}

void WebUiServer::handleExportHeader() {
  server_.sendHeader("Content-Disposition", "attachment; filename=\"Generated_LedMap.h\"");
  server_.send(200, "text/plain", store_.exportLedMapHeader());
}
