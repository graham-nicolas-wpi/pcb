#pragma once

#include <Arduino.h>
#include <RTClib.h>

#include "../model/ClockConfig.h"
#include "../render/SegmentFrame.h"

struct RendererHello {
  bool seen = false;
  String protocol;
  String canonicalProtocol;
  String name;
  String firmwareVersion;
  uint16_t pixels = 0;
  uint16_t logical = 0;
  bool rtcPresent = false;
  String control = "LOCAL";
  bool buttonReporting = false;
};

struct RendererStatus {
  bool seen = false;
  String mode = "CLOCK";
  String hourMode = "24H";
  String secondsMode = "BLINK";
  uint8_t brightness = 0;
  uint8_t colonBrightness = 0;
  uint32_t timerPresetSeconds = 0;
  uint32_t timerRemainingSeconds = 0;
  bool timerRunning = false;
  bool stopwatchRunning = false;
  String control = "LOCAL";
  String frame = "LOCAL";
  bool buttonReporting = false;
  bool mapValid = false;
  String protocol = "ClockRender/1";
};

struct RendererRtc {
  bool seen = false;
  bool present = false;
  bool lostPower = false;
  bool parsed = false;
  uint32_t sampledAtMs = 0;
  String summary = "unknown";
  DateTime dateTime = DateTime(2026, 1, 1, 0, 0, 0);
};

struct RendererCaps {
  bool seen = false;
  bool frame24 = false;
  bool control = false;
  bool buttonEvents = false;
  bool rtc = false;
  bool map = false;
  bool tests = false;
  String protocol = "ClockRender/1";
};

struct RendererMap {
  bool seen = false;
  uint8_t values[kNumPixels];
};

class LeonardoClient {
 public:
  explicit LeonardoClient(HardwareSerial& serial);

  void begin(const RendererLinkSettings& link);
  void poll(uint32_t nowMs);

  void requestHello();
  void requestCaps();
  void requestStatus();
  void requestRtc();
  void requestMap();
  void controlHost();
  void controlLocal();
  void setButtonReporting(bool enabled);
  void testDigits();
  void testSegments();
  void testAll();
  void setRtc(const DateTime& value);
  void sendFrame24(const SegmentFrame& frame);

  bool isOnline(uint32_t nowMs) const;
  uint32_t lastSeenMs() const;

  bool consumeButtonEvent(String& buttonId);
  bool consumeTimerDoneEvent();
  bool consumeHostTimeoutEvent();

  const RendererHello& hello() const;
  const RendererStatus& status() const;
  const RendererRtc& rtc() const;
  const RendererCaps& caps() const;
  const RendererMap& mapping() const;

 private:
  void sendCommand(const String& line);
  void handleLine(const String& line, uint32_t nowMs);

  HardwareSerial& serial_;
  String lineBuffer_;
  uint32_t lastSeenMs_ = 0;
  RendererHello hello_;
  RendererStatus status_;
  RendererRtc rtc_;
  RendererCaps caps_;
  RendererMap mapping_;
  uint8_t pendingButtonMask_ = 0;
  bool timerDoneLatched_ = false;
  bool hostTimeoutLatched_ = false;
};
