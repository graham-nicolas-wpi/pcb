#include "LeonardoClient.h"

#include "../../include/project_config.h"

#include <stdio.h>
#include <functional>

namespace {
void parseKeyValues(const String& text, std::function<void(const String&, const String&)> onPair) {
  int start = 0;
  while (start < text.length()) {
    while (start < text.length() && text[start] == ' ') {
      ++start;
    }
    if (start >= text.length()) {
      break;
    }

    int end = text.indexOf(' ', start);
    if (end < 0) {
      end = text.length();
    }
    const String token = text.substring(start, end);
    const int split = token.indexOf('=');
    if (split > 0) {
      onPair(token.substring(0, split), token.substring(split + 1));
    }
    start = end + 1;
  }
}

bool boolValue(const String& value) {
  return !(value == "0" || value == "false" || value == "FALSE" || value.length() == 0);
}

bool parseRtcSummary(const String& text, DateTime& out) {
  if (text.length() < 19) {
    return false;
  }
  const int year = text.substring(0, 4).toInt();
  const int month = text.substring(5, 7).toInt();
  const int day = text.substring(8, 10).toInt();
  const int hour = text.substring(11, 13).toInt();
  const int minute = text.substring(14, 16).toInt();
  const int second = text.substring(17, 19).toInt();
  out = DateTime(year, month, day, hour, minute, second);
  return true;
}
}

LeonardoClient::LeonardoClient(HardwareSerial& serial) : serial_(serial) {
  for (uint8_t& value : mapping_.values) {
    value = LOG_UNUSED;
  }
}

void LeonardoClient::begin(const RendererLinkSettings& link) {
  serial_.begin(link.baud, SERIAL_8N1, link.rxPin, link.txPin);
  lineBuffer_.reserve(256);
}

void LeonardoClient::sendCommand(const String& line) {
  serial_.print(line);
  serial_.print('\n');
}

void LeonardoClient::requestHello() { sendCommand("HELLO ClockRender/1"); }

void LeonardoClient::requestCaps() { sendCommand("CAPS?"); }

void LeonardoClient::requestStatus() { sendCommand("STATUS?"); }

void LeonardoClient::requestRtc() { sendCommand("RTC READ"); }

void LeonardoClient::requestMap() { sendCommand("MAP?"); }

void LeonardoClient::controlHost() { sendCommand("CONTROL HOST"); }

void LeonardoClient::controlLocal() { sendCommand("CONTROL LOCAL"); }

void LeonardoClient::setButtonReporting(bool enabled) {
  sendCommand(String("BTNREPORT ") + (enabled ? "1" : "0"));
}

void LeonardoClient::testDigits() { sendCommand("TEST DIGITS"); }

void LeonardoClient::testSegments() { sendCommand("TEST SEGMENTS"); }

void LeonardoClient::testAll() { sendCommand("TEST ALL"); }

void LeonardoClient::setRtc(const DateTime& value) {
  char buffer[40];
  snprintf(buffer, sizeof(buffer), "RTC SET %04u %02u %02u %02u %02u %02u", value.year(),
           value.month(), value.day(), value.hour(), value.minute(), value.second());
  sendCommand(buffer);
}

void LeonardoClient::sendFrame24(const SegmentFrame& frame) {
  String payload;
  frame.toFrame24Payload(payload);
  sendCommand(String("FRAME24 ") + payload);
}

void LeonardoClient::poll(uint32_t nowMs) {
  while (serial_.available()) {
    const char character = static_cast<char>(serial_.read());
    if (character == '\r') {
      continue;
    }
    if (character == '\n') {
      if (lineBuffer_.length() > 0U) {
        handleLine(lineBuffer_, nowMs);
        lineBuffer_ = "";
        lastSeenMs_ = nowMs;
      }
      continue;
    }
    if (lineBuffer_.length() < 255U) {
      lineBuffer_ += character;
    } else {
      lineBuffer_ = "";
    }
  }
}

void LeonardoClient::handleLine(const String& rawLine, uint32_t nowMs) {
  const String line = rawLine;

  if (line.startsWith("OK HELLO ")) {
    hello_.seen = true;
    const int protoEnd = line.indexOf(' ', 9);
    hello_.protocol = protoEnd < 0 ? line.substring(9) : line.substring(9, protoEnd);
    parseKeyValues(protoEnd < 0 ? "" : line.substring(protoEnd + 1),
                   [this](const String& key, const String& value) {
                     if (key == "NAME") hello_.name = value;
                     if (key == "FW") hello_.firmwareVersion = value;
                     if (key == "PIXELS") hello_.pixels = static_cast<uint16_t>(value.toInt());
                     if (key == "LOGICAL") hello_.logical = static_cast<uint16_t>(value.toInt());
                     if (key == "RTC") hello_.rtcPresent = boolValue(value);
                     if (key == "CONTROL") hello_.control = value;
                     if (key == "BTNREPORT") hello_.buttonReporting = boolValue(value);
                     if (key == "CANON") hello_.canonicalProtocol = value;
                   });
    return;
  }

  if (line.startsWith("OK CAPS ")) {
    caps_.seen = true;
    parseKeyValues(line.substring(8), [this](const String& key, const String& value) {
      if (key == "PROTO") caps_.protocol = value;
      if (key == "FRAME24") caps_.frame24 = boolValue(value);
      if (key == "CONTROL") caps_.control = boolValue(value);
      if (key == "BUTTON_EVENTS") caps_.buttonEvents = boolValue(value);
      if (key == "RTC") caps_.rtc = boolValue(value);
      if (key == "MAP") caps_.map = boolValue(value);
      if (key == "TESTS") caps_.tests = boolValue(value);
    });
    return;
  }

  if (line.startsWith("OK STATUS ")) {
    status_.seen = true;
    parseKeyValues(line.substring(10), [this](const String& key, const String& value) {
      if (key == "MODE") status_.mode = value;
      if (key == "HOURMODE") status_.hourMode = value;
      if (key == "SECONDSMODE") status_.secondsMode = value;
      if (key == "BRIGHTNESS") status_.brightness = static_cast<uint8_t>(value.toInt());
      if (key == "COLONBRIGHTNESS")
        status_.colonBrightness = static_cast<uint8_t>(value.toInt());
      if (key == "TIMER_PRESET")
        status_.timerPresetSeconds = static_cast<uint32_t>(value.toInt());
      if (key == "TIMER_REMAINING")
        status_.timerRemainingSeconds = static_cast<uint32_t>(value.toInt());
      if (key == "TIMER_RUNNING") status_.timerRunning = boolValue(value);
      if (key == "STOPWATCH_RUNNING") status_.stopwatchRunning = boolValue(value);
      if (key == "CONTROL") status_.control = value;
      if (key == "FRAME") status_.frame = value;
      if (key == "BTNREPORT") status_.buttonReporting = boolValue(value);
      if (key == "MAP_VALID") status_.mapValid = boolValue(value);
      if (key == "PROTO") status_.protocol = value;
    });
    return;
  }

  if (line.startsWith("MAP ")) {
    mapping_.seen = true;
    int physical = 0;
    int start = 4;
    while (physical < kNumPixels && start <= line.length()) {
      int comma = line.indexOf(',', start);
      if (comma < 0) {
        comma = line.length();
      }
      mapping_.values[physical++] = static_cast<uint8_t>(line.substring(start, comma).toInt());
      start = comma + 1;
    }
    return;
  }

  if (line == "RTC STATUS MISSING") {
    rtc_.seen = true;
    rtc_.present = false;
    rtc_.parsed = false;
    rtc_.sampledAtMs = nowMs;
    rtc_.summary = "missing";
    return;
  }

  if (line.startsWith("RTC ")) {
    rtc_.seen = true;
    int lostIndex = line.indexOf(" LOST_POWER=");
    String summary = lostIndex < 0 ? line.substring(4) : line.substring(4, lostIndex);
    rtc_.summary = summary;
    rtc_.present = true;
    rtc_.lostPower =
        lostIndex >= 0 ? boolValue(line.substring(lostIndex + 12)) : rtc_.lostPower;
    rtc_.parsed = parseRtcSummary(summary, rtc_.dateTime);
    rtc_.sampledAtMs = nowMs;
    return;
  }

  if (line == "EV TIMER DONE") {
    timerDoneLatched_ = true;
    return;
  }

  if (line == "EV HOST TIMEOUT") {
    hostTimeoutLatched_ = true;
    return;
  }

  if (line.startsWith("EV BUTTON ")) {
    if (line.indexOf("BTN1") > 0) pendingButtonMask_ |= 0x01U;
    if (line.indexOf("BTN2") > 0) pendingButtonMask_ |= 0x02U;
    if (line.indexOf("BTN3") > 0) pendingButtonMask_ |= 0x04U;
  }
}

bool LeonardoClient::isOnline(uint32_t nowMs) const {
  return hello_.seen && (nowMs - lastSeenMs_) < (ProjectConfig::kRendererControlRefreshMs * 3UL);
}

uint32_t LeonardoClient::lastSeenMs() const { return lastSeenMs_; }

bool LeonardoClient::consumeButtonEvent(String& buttonId) {
  if ((pendingButtonMask_ & 0x01U) != 0U) {
    pendingButtonMask_ &= static_cast<uint8_t>(~0x01U);
    buttonId = "BTN1";
    return true;
  }
  if ((pendingButtonMask_ & 0x02U) != 0U) {
    pendingButtonMask_ &= static_cast<uint8_t>(~0x02U);
    buttonId = "BTN2";
    return true;
  }
  if ((pendingButtonMask_ & 0x04U) != 0U) {
    pendingButtonMask_ &= static_cast<uint8_t>(~0x04U);
    buttonId = "BTN3";
    return true;
  }
  return false;
}

bool LeonardoClient::consumeTimerDoneEvent() {
  if (!timerDoneLatched_) {
    return false;
  }
  timerDoneLatched_ = false;
  return true;
}

bool LeonardoClient::consumeHostTimeoutEvent() {
  if (!hostTimeoutLatched_) {
    return false;
  }
  hostTimeoutLatched_ = false;
  return true;
}

const RendererHello& LeonardoClient::hello() const { return hello_; }

const RendererStatus& LeonardoClient::status() const { return status_; }

const RendererRtc& LeonardoClient::rtc() const { return rtc_; }

const RendererCaps& LeonardoClient::caps() const { return caps_; }

const RendererMap& LeonardoClient::mapping() const { return mapping_; }
