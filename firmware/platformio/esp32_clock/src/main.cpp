#include <Arduino.h>

#include "host/LeonardoClient.h"
#include "runtime/HostRuntime.h"
#include "storage/SettingsStore.h"
#include "web/WebUiServer.h"

SettingsStore gStore;
LeonardoClient gRenderer(Serial2);
HostRuntime gRuntime(gStore, gRenderer);
WebUiServer gWebUi(gStore, gRuntime);

void setup() {
  Serial.begin(115200);
  delay(80);

  gStore.begin();
  gRenderer.begin(gStore.config().rendererLink);

  gRuntime.begin();
  gWebUi.begin();

  Serial.println();
  Serial.println("OK BOOT ClockHost/1");
}

void loop() {
  const uint32_t nowMs = millis();
  gRuntime.update(nowMs);
  gWebUi.update();
}
