#pragma once

#include <Arduino.h>
#include <RTClib.h>

#include "../runtime/RendererRuntime.h"
#include "../util/Color.h"

class SettingsStore;
class AppStateMachine;
class Rtc_DS3231;
class LedMap;
class SegmentFrameRenderer;

class ClockRenderProtocol {
 public:
  ClockRenderProtocol(Stream& serial, SettingsStore& settings, AppStateMachine& app,
                      Rtc_DS3231& rtc, LedMap& map, RendererRuntime& runtime,
                      SegmentFrameRenderer& renderer, const char* firmwareName,
                      const char* firmwareVersion);

  void begin(size_t lineMax = 224U);
  void poll();
  void flushEvents();
  void sendHello(const char* protocolAlias = "ClockRender/1") const;
  void sendInfo() const;
  void sendCaps() const;
  void sendStatus() const;
  void sendRtcRead() const;

  static bool parseIntToken(const char* text, long& value);
  static bool parseRtcSet(const char* payload, DateTime& out);
  static bool parseRgbPayload(const char* payload, char* target, size_t targetSize,
                              RgbColor& newColor);

 private:
  bool handleLine(const String& line);
  bool handleRuntimeCommand(const String& line);
  void sendMap() const;
  void replyOK(const char* text) const;
  void replyOK(const __FlashStringHelper* text) const;
  void replyERR(const char* text) const;
  void replyERR(const __FlashStringHelper* text) const;
  const char* buttonName(Buttons::ButtonId buttonId) const;

  Stream& serial_;
  SettingsStore& settings_;
  AppStateMachine& app_;
  Rtc_DS3231& rtc_;
  LedMap& map_;
  RendererRuntime& runtime_;
  SegmentFrameRenderer& renderer_;
  const char* firmwareName_;
  const char* firmwareVersion_;
  size_t lineMax_;
  String lineBuffer_;
};
