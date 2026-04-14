#include "ClockRenderProtocol.h"

#include <stdlib.h>
#include <string.h>

#include "../display/LedMap.h"
#include "../display/LogicalIds.h"
#include "../hal/Rtc_DS3231.h"
#include "../render/SegmentFrame.h"
#include "../render/SegmentFrameRenderer.h"
#include "../settings/Profiles.h"
#include "../settings/SettingsStore.h"
#include "../ui/AppStateMachine.h"
#include "../ui/Modes.h"

namespace {
static constexpr size_t kColorTargetMax = 24U;

void printTwoDigits(Stream& serial, uint8_t value) {
  if (value < 10U) {
    serial.print('0');
  }
  serial.print(value);
}
}

ClockRenderProtocol::ClockRenderProtocol(Stream& serial, SettingsStore& settings,
                                         AppStateMachine& app, Rtc_DS3231& rtc, LedMap& map,
                                         RendererRuntime& runtime,
                                         SegmentFrameRenderer& renderer,
                                         const char* firmwareName,
                                         const char* firmwareVersion)
    : serial_(serial),
      settings_(settings),
      app_(app),
      rtc_(rtc),
      map_(map),
      runtime_(runtime),
      renderer_(renderer),
      firmwareName_(firmwareName),
      firmwareVersion_(firmwareVersion),
      lineMax_(224U) {}

void ClockRenderProtocol::begin(size_t lineMax) {
  lineMax_ = lineMax;
  lineBuffer_.reserve(lineMax_ + 1U);
}

void ClockRenderProtocol::replyOK(const __FlashStringHelper* text) const {
  serial_.print(F("OK "));
  serial_.println(text);
}

void ClockRenderProtocol::replyOK(const char* text) const {
  serial_.print(F("OK "));
  serial_.println(text);
}

void ClockRenderProtocol::replyERR(const __FlashStringHelper* text) const {
  serial_.print(F("ERR "));
  serial_.println(text);
}

void ClockRenderProtocol::replyERR(const char* text) const {
  serial_.print(F("ERR "));
  serial_.println(text);
}

void ClockRenderProtocol::poll() {
  while (serial_.available()) {
    const char c = static_cast<char>(serial_.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      if (lineBuffer_.length() > 0U) {
        handleLine(lineBuffer_);
        lineBuffer_ = "";
      }
      continue;
    }
    if (lineBuffer_.length() < lineMax_) {
      lineBuffer_ += c;
    } else {
      lineBuffer_ = "";
      replyERR(F("LINE TOO LONG"));
    }
  }
}

void ClockRenderProtocol::flushEvents() {
  ButtonEvent event;
  while (runtime_.consumeButtonEvent(event)) {
    serial_.print(F("EV BUTTON "));
    serial_.print(buttonName(event.buttonId));
    serial_.println(F(" SHORT"));
  }

  if (runtime_.consumeTimerDoneEvent()) {
    serial_.println(F("EV TIMER DONE"));
  }

  if (runtime_.consumeHostTimeoutEvent()) {
    serial_.println(F("EV HOST TIMEOUT"));
  }
}

void ClockRenderProtocol::sendHello(const char* protocolAlias) const {
  serial_.print(F("OK HELLO "));
  serial_.print(protocolAlias);
  serial_.print(F(" NAME="));
  serial_.print(firmwareName_);
  serial_.print(F(" FW="));
  serial_.print(firmwareVersion_);
  serial_.print(F(" PIXELS="));
  serial_.print(kNumPixels);
  serial_.print(F(" LOGICAL="));
  serial_.print(kLogicalCount);
  serial_.print(F(" RTC="));
  serial_.print(rtc_.isPresent() ? 1 : 0);
  serial_.print(F(" CONTROL="));
  serial_.print(runtime_.controlModeName());
  serial_.print(F(" BTNREPORT="));
  serial_.print(runtime_.buttonReportingEnabled() ? 1 : 0);
  serial_.print(F(" CANON=ClockRender/1"));
  serial_.println();
}

void ClockRenderProtocol::sendInfo() const {
  serial_.print(F("OK INFO BOARD=Leonardo PIXELS=31 PROTO=ClockRender/1"));
  serial_.print(F(" LEGACY=Clock/1"));
  serial_.print(F(" PIN=5"));
  serial_.print(F(" BTNS=8,9,10"));
  serial_.print(F(" RX=0 TX=1"));
  serial_.print(F(" CONTROL="));
  serial_.print(runtime_.controlModeName());
  serial_.print(F(" BTNREPORT="));
  serial_.println(runtime_.buttonReportingEnabled() ? 1 : 0);
}

void ClockRenderProtocol::sendCaps() const {
  serial_.println(
      F("OK CAPS PROTO=ClockRender/1 FRAME24=1 CONTROL=1 BUTTON_EVENTS=1 RTC=1 MAP=1 "
        "TESTS=1 LEGACY=Clock/1"));
}

void ClockRenderProtocol::sendStatus() const {
  const ClockSettings& settings = settings_.settings();
  serial_.print(F("OK STATUS MODE="));
  serial_.print(clockModeName(app_.mode()));
  serial_.print(F(" HOURMODE="));
  serial_.print(hourModeName(settings.hourMode));
  serial_.print(F(" SECONDSMODE="));
  serial_.print(secondsModeName(settings.secondsMode));
  serial_.print(F(" BRIGHTNESS="));
  serial_.print(settings.brightness);
  serial_.print(F(" COLONBRIGHTNESS="));
  serial_.print(settings.colonBrightness);
  serial_.print(F(" RAINBOW="));
  serial_.print(settings.rainbowClockMode ? 1 : 0);
  serial_.print(F(" TIMER_PRESET="));
  serial_.print(settings.timerPresetSeconds);
  serial_.print(F(" TIMERFORMAT="));
  serial_.print(timerDisplayFormatName(settings.timerDisplayFormat));
  serial_.print(F(" TIMER_REMAINING="));
  serial_.print(app_.timerRemainingSeconds());
  serial_.print(F(" TIMER_RUNNING="));
  serial_.print(app_.timerRunning() ? 1 : 0);
  serial_.print(F(" STOPWATCH_RUNNING="));
  serial_.print(app_.stopwatchRunning() ? 1 : 0);
  serial_.print(F(" CONTROL="));
  serial_.print(runtime_.controlModeName());
  serial_.print(F(" FRAME="));
  serial_.print(runtime_.hostFrameActive() ? F("FRAME24") : F("LOCAL"));
  serial_.print(F(" BTNREPORT="));
  serial_.print(runtime_.buttonReportingEnabled() ? 1 : 0);
  serial_.print(F(" MAP_VALID="));
  serial_.print(map_.validateNoDuplicates() ? 1 : 0);
  serial_.print(F(" PROTO=ClockRender/1"));
  serial_.println();
}

void ClockRenderProtocol::sendRtcRead() const {
  if (!rtc_.isPresent()) {
    serial_.println(F("RTC STATUS MISSING"));
    return;
  }

  const DateTime now = rtc_.now();
  serial_.print(F("RTC "));
  serial_.print(now.year());
  serial_.print('-');
  printTwoDigits(serial_, now.month());
  serial_.print('-');
  printTwoDigits(serial_, now.day());
  serial_.print(' ');
  printTwoDigits(serial_, now.hour());
  serial_.print(':');
  printTwoDigits(serial_, now.minute());
  serial_.print(':');
  printTwoDigits(serial_, now.second());
  serial_.print(F(" LOST_POWER="));
  serial_.println(rtc_.lostPower() ? 1 : 0);
}

void ClockRenderProtocol::sendMap() const {
  serial_.print(F("MAP "));
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    serial_.print(map_.physToLogical[index]);
    if (index + 1U < kNumPixels) {
      serial_.print(',');
    }
  }
  serial_.println();
}

bool ClockRenderProtocol::parseIntToken(const char* text, long& value) {
  if (text == nullptr || text[0] == '\0') {
    return false;
  }
  char* end = nullptr;
  value = strtol(text, &end, 10);
  return end != nullptr && *end == '\0';
}

bool ClockRenderProtocol::parseRtcSet(const char* payload, DateTime& out) {
  char buffer[40];
  const size_t payloadLen = strlen(payload);
  if (payloadLen == 0U || payloadLen >= sizeof(buffer)) {
    return false;
  }
  memcpy(buffer, payload, payloadLen + 1U);

  int values[6] = {0, 0, 0, 0, 0, 0};
  char* cursor = buffer;
  for (uint8_t i = 0; i < 6; ++i) {
    while (*cursor == ' ') {
      ++cursor;
    }
    if (*cursor == '\0') {
      return false;
    }

    char* tokenEnd = cursor;
    while (*tokenEnd != '\0' && *tokenEnd != ' ') {
      ++tokenEnd;
    }
    const char saved = *tokenEnd;
    *tokenEnd = '\0';

    long parsed = 0;
    if (!parseIntToken(cursor, parsed)) {
      return false;
    }
    values[i] = static_cast<int>(parsed);
    cursor = tokenEnd;
    if (saved != '\0') {
      ++cursor;
    }
  }

  while (*cursor == ' ') {
    ++cursor;
  }
  if (*cursor != '\0') {
    return false;
  }

  out = DateTime(values[0], values[1], values[2], values[3], values[4], values[5]);
  return true;
}

bool ClockRenderProtocol::parseRgbPayload(const char* payload, char* target, size_t targetSize,
                                          RgbColor& newColor) {
  while (*payload == ' ') {
    ++payload;
  }
  if (*payload == '\0') {
    return false;
  }

  const char* targetEnd = payload;
  while (*targetEnd != '\0' && *targetEnd != ' ') {
    ++targetEnd;
  }
  const size_t targetLen = static_cast<size_t>(targetEnd - payload);
  if (targetLen == 0U || targetLen >= targetSize) {
    return false;
  }
  memcpy(target, payload, targetLen);
  target[targetLen] = '\0';

  payload = targetEnd;
  long values[3] = {0, 0, 0};
  for (uint8_t index = 0; index < 3U; ++index) {
    while (*payload == ' ') {
      ++payload;
    }
    if (*payload == '\0') {
      return false;
    }

    char token[6];
    size_t tokenLen = 0U;
    while (payload[tokenLen] != '\0' && payload[tokenLen] != ' ') {
      if (tokenLen + 1U >= sizeof(token)) {
        return false;
      }
      token[tokenLen] = payload[tokenLen];
      ++tokenLen;
    }
    token[tokenLen] = '\0';

    if (!parseIntToken(token, values[index])) {
      return false;
    }
    values[index] = constrain(values[index], 0L, 255L);
    payload += tokenLen;
  }

  while (*payload == ' ') {
    ++payload;
  }
  if (*payload != '\0') {
    return false;
  }

  newColor = makeRgbColor(static_cast<uint8_t>(values[0]), static_cast<uint8_t>(values[1]),
                          static_cast<uint8_t>(values[2]));
  return true;
}

const char* ClockRenderProtocol::buttonName(Buttons::ButtonId buttonId) const {
  switch (buttonId) {
    case Buttons::ButtonId::BTN1:
      return "BTN1";
    case Buttons::ButtonId::BTN2:
      return "BTN2";
    case Buttons::ButtonId::BTN3:
      return "BTN3";
  }
  return "BTN1";
}

bool ClockRenderProtocol::handleLine(const String& rawLine) {
  String line = rawLine;
  line.trim();
  if (line.length() == 0U) {
    return false;
  }

  const uint32_t nowMs = millis();
  runtime_.noteHostActivity(nowMs);

  if (line == "HELLO") {
    sendHello();
    return true;
  }

  if (line == "HELLO ClockRender/1") {
    sendHello("ClockRender/1");
    return true;
  }

  if (line == "HELLO Clock/1") {
    sendHello("Clock/1");
    return true;
  }

  if (line == "HELLO ClockCal/1") {
    replyERR(F("USE CALIBRATION FIRMWARE FOR ClockCal/1"));
    return true;
  }

  if (line == "INFO?" || line == "GET INFO") {
    sendInfo();
    return true;
  }

  if (line == "CAPS?") {
    sendCaps();
    return true;
  }

  if (line == "STATUS?" || line == "GET STATUS") {
    sendStatus();
    return true;
  }

  if (line == "PING") {
    replyOK(F("PONG"));
    return true;
  }

  return handleRuntimeCommand(line);
}

bool ClockRenderProtocol::handleRuntimeCommand(const String& line) {
  ClockSettings& settings = settings_.mutableSettings();
  const char* rawLine = line.c_str();
  const uint32_t nowMs = millis();

  if (line == "CONTROL HOST") {
    runtime_.setControlMode(RendererControlMode::HOST, nowMs);
    replyOK(F("CONTROL HOST"));
    return true;
  }

  if (line == "CONTROL LOCAL") {
    runtime_.setControlMode(RendererControlMode::LOCAL, nowMs);
    replyOK(F("CONTROL LOCAL"));
    return true;
  }

  if (line.startsWith("BTNREPORT ")) {
    long value = 0;
    if (!parseIntToken(rawLine + 10, value)) {
      replyERR(F("BTNREPORT"));
      return true;
    }
    runtime_.setButtonReporting(value != 0L);
    serial_.print(F("OK BTNREPORT "));
    serial_.println(runtime_.buttonReportingEnabled() ? 1 : 0);
    return true;
  }

  if (line == "RTC READ") {
    sendRtcRead();
    return true;
  }

  if (line.startsWith("RTC SET ")) {
    if (!rtc_.isPresent()) {
      replyERR(F("RTC MISSING"));
      return true;
    }

    DateTime dateTime;
    if (!parseRtcSet(rawLine + 8, dateTime)) {
      replyERR(F("RTC FORMAT"));
      return true;
    }

    rtc_.adjust(dateTime);
    runtime_.useLocalRenderer(nowMs);
    replyOK(F("RTC SET"));
    sendRtcRead();
    return true;
  }

  if (line == "MAP?") {
    sendMap();
    return true;
  }

  if (line == "VALIDATE?") {
    serial_.println(map_.validateNoDuplicates() ? F("OK VALIDATE PASS")
                                                : F("OK VALIDATE FAIL"));
    return true;
  }

  if (line == "TEST DIGITS") {
    renderer_.testDigits();
    runtime_.requestRender();
    replyOK(F("TEST DIGITS"));
    return true;
  }

  if (line == "TEST SEGMENTS") {
    renderer_.testSegments();
    runtime_.requestRender();
    replyOK(F("TEST SEGMENTS"));
    return true;
  }

  if (line == "TEST ALL") {
    renderer_.testAll();
    runtime_.requestRender();
    replyOK(F("TEST ALL"));
    return true;
  }

  if (line.startsWith("FRAME24 ")) {
    SegmentFrame frame;
    if (!frame.fromFrame24Payload(rawLine + 8)) {
      replyERR(F("FRAME24 FORMAT"));
      return true;
    }
    runtime_.setControlMode(RendererControlMode::HOST, nowMs);
    runtime_.setHostFrame(frame, nowMs);
    replyOK(F("FRAME24"));
    return true;
  }

  if (line == "MODE CLOCK_SECONDS") {
    runtime_.useLocalRenderer(nowMs);
    app_.setMode(ClockMode::CLOCK_SECONDS);
    settings_.setMode(ClockMode::CLOCK_SECONDS);
    runtime_.requestRender();
    replyOK(F("MODE CLOCK_SECONDS"));
    return true;
  }

  if (line == "MODE CLOCK") {
    runtime_.useLocalRenderer(nowMs);
    app_.setMode(ClockMode::CLOCK);
    settings_.setMode(ClockMode::CLOCK);
    runtime_.requestRender();
    replyOK(F("MODE CLOCK"));
    return true;
  }

  if (line == "MODE TIMER") {
    runtime_.useLocalRenderer(nowMs);
    app_.setMode(ClockMode::TIMER);
    settings_.setMode(ClockMode::TIMER);
    runtime_.requestRender();
    replyOK(F("MODE TIMER"));
    return true;
  }

  if (line == "MODE STOPWATCH") {
    runtime_.useLocalRenderer(nowMs);
    app_.setMode(ClockMode::STOPWATCH);
    settings_.setMode(ClockMode::STOPWATCH);
    runtime_.requestRender();
    replyOK(F("MODE STOPWATCH"));
    return true;
  }

  if (line == "MODE COLOR_DEMO") {
    runtime_.useLocalRenderer(nowMs);
    app_.setMode(ClockMode::COLOR_DEMO);
    settings_.setMode(ClockMode::COLOR_DEMO);
    runtime_.requestRender();
    replyOK(F("MODE COLOR_DEMO"));
    return true;
  }

  if (line == "HOURMODE 24H") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setHourMode(HourMode::H24);
    runtime_.requestRender();
    replyOK(F("HOURMODE 24H"));
    return true;
  }

  if (line == "HOURMODE 12H") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setHourMode(HourMode::H12);
    runtime_.requestRender();
    replyOK(F("HOURMODE 12H"));
    return true;
  }

  if (line == "SECONDSMODE BLINK") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setSecondsDisplayMode(SecondsDisplayMode::BLINK);
    runtime_.requestRender();
    replyOK(F("SECONDSMODE BLINK"));
    return true;
  }

  if (line == "SECONDSMODE ON") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setSecondsDisplayMode(SecondsDisplayMode::ON);
    runtime_.requestRender();
    replyOK(F("SECONDSMODE ON"));
    return true;
  }

  if (line == "SECONDSMODE OFF") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setSecondsDisplayMode(SecondsDisplayMode::OFF);
    runtime_.requestRender();
    replyOK(F("SECONDSMODE OFF"));
    return true;
  }

  if (line == "TIMERFORMAT AUTO") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setTimerDisplayFormat(TimerDisplayFormat::AUTO);
    runtime_.requestRender();
    replyOK(F("TIMERFORMAT AUTO"));
    return true;
  }

  if (line == "TIMERFORMAT HHMM") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setTimerDisplayFormat(TimerDisplayFormat::HH_MM);
    runtime_.requestRender();
    replyOK(F("TIMERFORMAT HHMM"));
    return true;
  }

  if (line == "TIMERFORMAT MMSS") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setTimerDisplayFormat(TimerDisplayFormat::MM_SS);
    runtime_.requestRender();
    replyOK(F("TIMERFORMAT MMSS"));
    return true;
  }

  if (line == "TIMERFORMAT SSCS") {
    runtime_.useLocalRenderer(nowMs);
    settings_.setTimerDisplayFormat(TimerDisplayFormat::SS_CS);
    runtime_.requestRender();
    replyOK(F("TIMERFORMAT SSCS"));
    return true;
  }

  if (line == "PRESET RAINBOW" || line == "PRESET PARTY") {
    const char* presetName = rawLine + 7;
    if (!applyPresetByName(presetName, settings)) {
      replyERR(F("PRESET"));
      return true;
    }
    runtime_.useLocalRenderer(nowMs);
    settings_.markDirty();
    runtime_.requestRender();
    serial_.print(F("OK PRESET "));
    serial_.println(presetName);
    return true;
  }

  if (line.startsWith("COLONBRIGHTNESS ")) {
    long value = 0;
    if (!parseIntToken(rawLine + 16, value)) {
      replyERR(F("COLONBRIGHTNESS"));
      return true;
    }
    runtime_.useLocalRenderer(nowMs);
    settings_.setColonBrightness(static_cast<uint8_t>(constrain(value, 0L, 255L)));
    runtime_.requestRender();
    serial_.print(F("OK COLONBRIGHTNESS "));
    serial_.println(settings_.settings().colonBrightness);
    return true;
  }

  if (line.startsWith("BRIGHTNESS ")) {
    long value = 0;
    if (!parseIntToken(rawLine + 11, value)) {
      replyERR(F("BRIGHTNESS"));
      return true;
    }
    runtime_.useLocalRenderer(nowMs);
    settings_.setBrightness(static_cast<uint8_t>(constrain(value, 1L, 255L)));
    runtime_.requestRender();
    serial_.print(F("OK BRIGHTNESS "));
    serial_.println(settings_.settings().brightness);
    return true;
  }

  if (line.startsWith("COLOR ")) {
    char target[kColorTargetMax];
    RgbColor newColor = makeRgbColor(0, 0, 0);
    if (!parseRgbPayload(rawLine + 6, target, sizeof(target), newColor)) {
      replyERR(F("COLOR FORMAT"));
      return true;
    }
    if (!applyColorTarget(target, newColor, settings)) {
      replyERR(F("COLOR TARGET"));
      return true;
    }
    runtime_.useLocalRenderer(nowMs);
    settings_.markDirty();
    runtime_.requestRender();
    serial_.print(F("OK COLOR "));
    serial_.println(target);
    return true;
  }

  if (line.startsWith("TIMER SET ")) {
    long value = 0;
    if (!parseIntToken(rawLine + 10, value)) {
      replyERR(F("TIMER SET"));
      return true;
    }
    if (value < 0L) {
      value = 0L;
    }
    runtime_.useLocalRenderer(nowMs);
    settings_.setTimerPresetSeconds(static_cast<uint32_t>(value));
    app_.resetTimer(settings_.settings().timerPresetSeconds);
    runtime_.requestRender();
    serial_.print(F("OK TIMER SET "));
    serial_.println(settings_.settings().timerPresetSeconds);
    return true;
  }

  if (line == "TIMER START") {
    runtime_.useLocalRenderer(nowMs);
    app_.startTimer(nowMs, settings.timerPresetSeconds);
    settings_.setMode(ClockMode::TIMER);
    runtime_.requestRender();
    replyOK(F("TIMER START"));
    return true;
  }

  if (line == "TIMER STOP") {
    runtime_.useLocalRenderer(nowMs);
    app_.stopTimer(nowMs);
    runtime_.requestRender();
    replyOK(F("TIMER STOP"));
    return true;
  }

  if (line == "TIMER RESET") {
    runtime_.useLocalRenderer(nowMs);
    app_.resetTimer(settings.timerPresetSeconds);
    runtime_.requestRender();
    replyOK(F("TIMER RESET"));
    return true;
  }

  if (line == "STOPWATCH START") {
    runtime_.useLocalRenderer(nowMs);
    app_.startStopwatch(nowMs);
    settings_.setMode(ClockMode::STOPWATCH);
    runtime_.requestRender();
    replyOK(F("STOPWATCH START"));
    return true;
  }

  if (line == "STOPWATCH STOP") {
    runtime_.useLocalRenderer(nowMs);
    app_.stopStopwatch(nowMs);
    runtime_.requestRender();
    replyOK(F("STOPWATCH STOP"));
    return true;
  }

  if (line == "STOPWATCH RESET") {
    runtime_.useLocalRenderer(nowMs);
    app_.resetStopwatch(nowMs);
    runtime_.requestRender();
    replyOK(F("STOPWATCH RESET"));
    return true;
  }

  if (line == "SAVE") {
    settings_.save();
    replyOK(F("SAVE"));
    return true;
  }

  if (line == "LOAD") {
    settings_.load();
    app_.begin(settings_.settings(), nowMs);
    runtime_.useLocalRenderer(nowMs);
    replyOK(F("LOAD"));
    return true;
  }

  serial_.print(F("ERR UNKNOWN "));
  serial_.println(line);
  return true;
}
