#pragma once

#include <Arduino.h>
#include <RTClib.h>

#include "../util/Color.h"

class SettingsStore;
class AppStateMachine;
class Rtc_DS3231;

class ClockProtocol {
 public:
  ClockProtocol(Stream& serial, SettingsStore& settings, AppStateMachine& app, Rtc_DS3231& rtc,
                const char* firmwareName, const char* firmwareVersion);

  void begin(size_t lineMax = 96U);
  void poll();
  bool handleLine(char* rawLine);  // trims in place

  bool consumeRenderRequested();
  void sendFirmwareInfo() const;
  void sendStatus() const;
  void sendRtcRead() const;
  void sendTimerDone();
  void replyOK(const char* text) const;
  void replyOK(const __FlashStringHelper* text) const;
  void replyERR(const char* text) const;
  void replyERR(const __FlashStringHelper* text) const;

  static bool parseIntToken(const char* text, long& value);
  static bool parseRtcSet(const char* payload, DateTime& out);
  static bool parseRgbPayload(const char* payload, char* target, size_t targetSize,
                              RgbColor& newColor);

 private:
  bool handleClockCommand(const char* rawLine);

  Stream& serial_;
  SettingsStore& settings_;
  AppStateMachine& app_;
  Rtc_DS3231& rtc_;
  const char* firmwareName_;
  const char* firmwareVersion_;
  static constexpr size_t kLineBufSize = 97U;
  size_t lineMax_;
  char lineBuffer_[kLineBufSize];
  size_t lineLen_;
  bool renderRequested_;
};
