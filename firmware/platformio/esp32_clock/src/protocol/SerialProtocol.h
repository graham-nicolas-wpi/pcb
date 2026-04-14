#pragma once

#include <Arduino.h>

#include "../runtime/ClockRuntime.h"
#include "../storage/SettingsStore.h"

class SerialProtocol {
 public:
  SerialProtocol(Stream& serial, SettingsStore& store, ClockRuntime& runtime);

  void begin(size_t lineMax = 160U);
  void poll();

 private:
  void handleLine(const String& line);
  void sendHello(const char* protocolName) const;
  void sendInfo() const;
  void sendStatus() const;
  void sendMap() const;
  void sendRtc() const;
  void replyOK(const String& text) const;
  void replyERR(const String& text) const;
  bool parseRtc(const String& payload, DateTime& out) const;
  bool parseColor(const String& payload, String& target, RgbColor& color) const;
  int resolvePresetIndex(const String& token) const;

  Stream& serial_;
  SettingsStore& store_;
  ClockRuntime& runtime_;
  String lineBuffer_;
  size_t lineMax_ = 160U;
};
