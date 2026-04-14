#include "ClockProtocol.h"

#include <string.h>

#include "../display/LogicalIds.h"
#include "../hal/Rtc_DS3231.h"
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

ClockProtocol::ClockProtocol(Stream& serial, SettingsStore& settings, AppStateMachine& app,
                             Rtc_DS3231& rtc, const char* firmwareName,
                             const char* firmwareVersion)
    : serial_(serial),
      settings_(settings),
      app_(app),
      rtc_(rtc),
      firmwareName_(firmwareName),
      firmwareVersion_(firmwareVersion),
      lineMax_(96U),
      renderRequested_(false) {}

void ClockProtocol::begin(size_t lineMax) {
  lineMax_ = lineMax;
  lineBuffer_.reserve(lineMax_ + 1U);
}

void ClockProtocol::poll() {
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

bool ClockProtocol::consumeRenderRequested() {
  if (!renderRequested_) {
    return false;
  }
  renderRequested_ = false;
  return true;
}

void ClockProtocol::replyOK(const __FlashStringHelper* text) const {
  serial_.print(F("OK "));
  serial_.println(text);
}

void ClockProtocol::replyOK(const char* text) const {
  serial_.print(F("OK "));
  serial_.println(text);
}

void ClockProtocol::replyERR(const __FlashStringHelper* text) const {
  serial_.print(F("ERR "));
  serial_.println(text);
}

void ClockProtocol::replyERR(const char* text) const {
  serial_.print(F("ERR "));
  serial_.println(text);
}

void ClockProtocol::sendFirmwareInfo() const {
  serial_.print(F("OK HELLO Clock/1 NAME="));
  serial_.print(firmwareName_);
  serial_.print(F(" FW="));
  serial_.print(firmwareVersion_);
  serial_.print(F(" PIXELS="));
  serial_.print(kNumPixels);
  serial_.print(F(" LOGICAL="));
  serial_.print(kLogicalCount);
  serial_.print(F(" CAL="));
  serial_.print(0);
  serial_.print(F(" RTC="));
  serial_.println(rtc_.isPresent() ? 1 : 0);
}

void ClockProtocol::sendStatus() const {
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
  serial_.print(F(" CAL="));
  serial_.println(0);
}

void ClockProtocol::sendRtcRead() const {
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

void ClockProtocol::sendTimerDone() {
  replyOK(F("TIMER DONE"));
}

bool ClockProtocol::parseIntToken(const char* text, long& value) {
  if (text == NULL || text[0] == '\0') {
    return false;
  }
  char* end = NULL;
  value = strtol(text, &end, 10);
  return end != NULL && *end == '\0';
}

bool ClockProtocol::parseRtcSet(const char* payload, DateTime& out) {
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

bool ClockProtocol::parseRgbPayload(const char* payload, char* target, size_t targetSize,
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
  for (uint8_t i = 0; i < 3; ++i) {
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

    if (!parseIntToken(token, values[i])) {
      return false;
    }
    values[i] = constrain(values[i], 0L, 255L);
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

bool ClockProtocol::handleLine(const String& rawLine) {
  String line = rawLine;
  line.trim();
  if (line.length() == 0U) {
    return false;
  }

  if (line == "HELLO" || line == "HELLO Clock/1" || line == "HELLO ClockCal/1" ||
      line == "INFO?" || line == "GET INFO") {
    sendFirmwareInfo();
    return true;
  }

  if (line == "STATUS?" || line == "GET STATUS") {
    sendStatus();
    return true;
  }

  return handleClockCommand(line);
}

bool ClockProtocol::handleClockCommand(const String& line) {
  ClockSettings& settings = settings_.mutableSettings();
  const char* rawLine = line.c_str();

  if (line == "MODE CLOCK_SECONDS") {
    app_.setMode(ClockMode::CLOCK_SECONDS);
    settings_.setMode(ClockMode::CLOCK_SECONDS);
    renderRequested_ = true;
    replyOK(F("MODE CLOCK_SECONDS"));
    return true;
  }

  if (line == "MODE CLOCK") {
    app_.setMode(ClockMode::CLOCK);
    settings_.setMode(ClockMode::CLOCK);
    renderRequested_ = true;
    replyOK(F("MODE CLOCK"));
    return true;
  }

  if (line == "MODE TIMER") {
    app_.setMode(ClockMode::TIMER);
    settings_.setMode(ClockMode::TIMER);
    renderRequested_ = true;
    replyOK(F("MODE TIMER"));
    return true;
  }

  if (line == "MODE STOPWATCH") {
    app_.setMode(ClockMode::STOPWATCH);
    settings_.setMode(ClockMode::STOPWATCH);
    renderRequested_ = true;
    replyOK(F("MODE STOPWATCH"));
    return true;
  }

  if (line == "MODE COLOR_DEMO") {
    app_.setMode(ClockMode::COLOR_DEMO);
    settings_.setMode(ClockMode::COLOR_DEMO);
    renderRequested_ = true;
    replyOK(F("MODE COLOR_DEMO"));
    return true;
  }

  if (line == "HOURMODE 24H") {
    settings_.setHourMode(HourMode::H24);
    renderRequested_ = true;
    replyOK(F("HOURMODE 24H"));
    return true;
  }

  if (line == "HOURMODE 12H") {
    settings_.setHourMode(HourMode::H12);
    renderRequested_ = true;
    replyOK(F("HOURMODE 12H"));
    return true;
  }

  if (line == "SECONDSMODE BLINK") {
    settings_.setSecondsDisplayMode(SecondsDisplayMode::BLINK);
    renderRequested_ = true;
    replyOK(F("SECONDSMODE BLINK"));
    return true;
  }

  if (line == "SECONDSMODE ON") {
    settings_.setSecondsDisplayMode(SecondsDisplayMode::ON);
    renderRequested_ = true;
    replyOK(F("SECONDSMODE ON"));
    return true;
  }

  if (line == "SECONDSMODE OFF") {
    settings_.setSecondsDisplayMode(SecondsDisplayMode::OFF);
    renderRequested_ = true;
    replyOK(F("SECONDSMODE OFF"));
    return true;
  }

  if (line == "TIMERFORMAT AUTO") {
    settings_.setTimerDisplayFormat(TimerDisplayFormat::AUTO);
    renderRequested_ = true;
    replyOK(F("TIMERFORMAT AUTO"));
    return true;
  }

  if (line == "TIMERFORMAT HHMM") {
    settings_.setTimerDisplayFormat(TimerDisplayFormat::HH_MM);
    renderRequested_ = true;
    replyOK(F("TIMERFORMAT HHMM"));
    return true;
  }

  if (line == "TIMERFORMAT MMSS") {
    settings_.setTimerDisplayFormat(TimerDisplayFormat::MM_SS);
    renderRequested_ = true;
    replyOK(F("TIMERFORMAT MMSS"));
    return true;
  }

  if (line == "TIMERFORMAT SSCS") {
    settings_.setTimerDisplayFormat(TimerDisplayFormat::SS_CS);
    renderRequested_ = true;
    replyOK(F("TIMERFORMAT SSCS"));
    return true;
  }

  if (line == "PRESET RAINBOW" || line == "PRESET PARTY") {
    const char* presetName = rawLine + 7;
    if (!applyPresetByName(presetName, settings)) {
      replyERR(F("PRESET"));
      return true;
    }
    settings_.markDirty();
    renderRequested_ = true;
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
    settings_.setColonBrightness(static_cast<uint8_t>(constrain(value, 0L, 255L)));
    renderRequested_ = true;
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
    settings_.setBrightness(static_cast<uint8_t>(constrain(value, 1L, 255L)));
    renderRequested_ = true;
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
    settings_.markDirty();
    renderRequested_ = true;
    serial_.print(F("OK COLOR "));
    serial_.println(target);
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
    replyOK(F("RTC SET"));
    sendRtcRead();
    renderRequested_ = true;
    return true;
  }

  if (line.startsWith("TIMER SET ")) {
    long value = 0;
    if (!parseIntToken(rawLine + 10, value)) {
      replyERR(F("TIMER SET"));
      return true;
    }
    if (value < 0) {
      value = 0;
    }
    settings_.setTimerPresetSeconds(static_cast<uint32_t>(value));
    app_.resetTimer(settings_.settings().timerPresetSeconds);
    renderRequested_ = true;
    serial_.print(F("OK TIMER SET "));
    serial_.println(settings_.settings().timerPresetSeconds);
    return true;
  }

  if (line == "TIMER START") {
    app_.startTimer(millis(), settings.timerPresetSeconds);
    settings_.setMode(ClockMode::TIMER);
    renderRequested_ = true;
    replyOK(F("TIMER START"));
    return true;
  }

  if (line == "TIMER STOP") {
    app_.stopTimer(millis());
    renderRequested_ = true;
    replyOK(F("TIMER STOP"));
    return true;
  }

  if (line == "TIMER RESET") {
    app_.resetTimer(settings.timerPresetSeconds);
    renderRequested_ = true;
    replyOK(F("TIMER RESET"));
    return true;
  }

  if (line == "STOPWATCH START") {
    app_.startStopwatch(millis());
    settings_.setMode(ClockMode::STOPWATCH);
    renderRequested_ = true;
    replyOK(F("STOPWATCH START"));
    return true;
  }

  if (line == "STOPWATCH STOP") {
    app_.stopStopwatch(millis());
    renderRequested_ = true;
    replyOK(F("STOPWATCH STOP"));
    return true;
  }

  if (line == "STOPWATCH RESET") {
    app_.resetStopwatch(millis());
    renderRequested_ = true;
    replyOK(F("STOPWATCH RESET"));
    return true;
  }

  serial_.print(F("ERR UNKNOWN "));
  serial_.println(line);
  return true;
}
