#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "../runtime/HostRuntime.h"

// clockWinder over WiFi. The desktop GUI connects to tcp://<hostname>.local:3333
// (pyserial "socket://" URL) and speaks the same newline-delimited
// ClockRender/1 commands it uses over USB. Commands are applied to the ESP32
// host model, so the 32U4 stays a pure renderer underneath.
class WinderServer {
 public:
  explicit WinderServer(HostRuntime& runtime);

  void begin(uint16_t port);
  void update();

 private:
  void handleLine(const String& line);
  void reply(const String& line);
  void sendHello(const String& alias);
  void sendInfo();
  void sendStatus();
  void sendRtc();
  void sendMap();

  HostRuntime& runtime_;
  WiFiServer server_;
  WiFiClient client_;
  String buffer_;
  bool started_ = false;
};
