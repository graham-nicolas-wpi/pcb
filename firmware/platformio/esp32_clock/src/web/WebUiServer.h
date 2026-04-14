#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "../runtime/ClockRuntime.h"
#include "../storage/SettingsStore.h"

class WebUiServer {
 public:
  WebUiServer(SettingsStore& store, ClockRuntime& runtime);

  void begin();
  void update();

 private:
  void configureWiFi();
  void registerRoutes();
  void handleBootstrap();
  void handleConfigSave();
  void handleAction();
  void handleExportConfig();
  void handleExportMapping();
  void handleExportHeader();

  bool readJsonBody(DynamicJsonDocument& doc);
  void sendJsonDocument(const DynamicJsonDocument& doc, int statusCode = 200);
  void appendBootstrapJson(JsonObject root);

  SettingsStore& store_;
  ClockRuntime& runtime_;
  WebServer server_;
};
