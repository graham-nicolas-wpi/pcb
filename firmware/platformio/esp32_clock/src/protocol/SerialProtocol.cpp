#include "SerialProtocol.h"

#include "../../include/project_config.h"

SerialProtocol::SerialProtocol(Stream& serial, SettingsStore& store, ClockRuntime& runtime)
    : serial_(serial), store_(store), runtime_(runtime) {}

void SerialProtocol::begin(size_t lineMax) {
  lineMax_ = lineMax;
  lineBuffer_.reserve(lineMax_ + 1U);
}

void SerialProtocol::poll() {
  while (serial_.available()) {
    const char character = static_cast<char>(serial_.read());
    if (character == '\r') {
      continue;
    }
    if (character == '\n') {
      if (lineBuffer_.length() > 0U) {
        handleLine(lineBuffer_);
        lineBuffer_ = "";
      }
      continue;
    }
    if (lineBuffer_.length() < lineMax_) {
      lineBuffer_ += character;
    } else {
      lineBuffer_ = "";
      replyERR("LINE TOO LONG");
    }
  }
}

void SerialProtocol::replyOK(const String& text) const {
  serial_.print("OK ");
  serial_.println(text);
}

void SerialProtocol::replyERR(const String& text) const {
  serial_.print("ERR ");
  serial_.println(text);
}

void SerialProtocol::sendHello(const char* protocolName) const {
  serial_.print("OK HELLO ");
  serial_.print(protocolName);
  serial_.print(" NAME=");
  serial_.print(CLOCK_DEVICE_NAME);
  serial_.print(" FW=");
  serial_.print(ProjectConfig::kFirmwareVersion);
  serial_.print(" PIXELS=");
  serial_.print(store_.config().hardware.pixelCount);
  serial_.print(" LOGICAL=");
  serial_.print(kLogicalCount);
  serial_.print(" CAL=");
  serial_.print(runtime_.calibrationMode() ? 1 : 0);
  serial_.print(" RTC=");
  serial_.println(runtime_.rtcPresent() ? 1 : 0);
}

void SerialProtocol::sendInfo() const {
  serial_.print("OK INFO BOARD=ESP32-WROOM-32E PIN=");
  serial_.print(store_.config().hardware.pixelPin);
  serial_.print(" PIXELS=");
  serial_.print(store_.config().hardware.pixelCount);
  serial_.print(" PROTO=ClockESP/1 BTNS=");
  serial_.print(store_.config().hardware.buttonPins[0]);
  serial_.print(',');
  serial_.print(store_.config().hardware.buttonPins[1]);
  serial_.print(',');
  serial_.println(store_.config().hardware.buttonPins[2]);
}

void SerialProtocol::sendStatus() const {
  serial_.print("OK STATUS MODE=");
  serial_.print(clockModeName(store_.config().runtime.mode));
  serial_.print(" HOURMODE=");
  serial_.print(hourModeName(store_.config().runtime.hourMode));
  serial_.print(" SECONDSMODE=");
  serial_.print(secondsModeName(store_.config().runtime.secondsMode));
  serial_.print(" BRIGHTNESS=");
  serial_.print(store_.config().runtime.brightness);
  serial_.print(" COLONBRIGHTNESS=");
  serial_.print(store_.config().runtime.colonBrightness);
  serial_.print(" RAINBOW=");
  serial_.print(store_.config().runtime.rainbowEnabled ? 1 : 0);
  serial_.print(" TIMER_PRESET=");
  serial_.print(store_.config().runtime.timerPresetSeconds);
  serial_.print(" TIMERFORMAT=");
  serial_.print(timerFormatName(store_.config().runtime.timerFormat));
  serial_.print(" TIMER_REMAINING=");
  serial_.print(runtime_.timerRemainingSeconds());
  serial_.print(" TIMER_RUNNING=");
  serial_.print(runtime_.timerRunning() ? 1 : 0);
  serial_.print(" STOPWATCH_RUNNING=");
  serial_.print(runtime_.stopwatchRunning() ? 1 : 0);
  serial_.print(" CAL=");
  serial_.print(runtime_.calibrationMode() ? 1 : 0);
  serial_.print(" CUR=");
  serial_.print(runtime_.calibrationCursor());
  serial_.print(" VALID=");
  serial_.print(runtime_.validateMapping() ? 1 : 0);
  serial_.print(" ACTIVEPRESET=");
  serial_.print(store_.config().runtime.activePresetId);
  serial_.print(" ACTIVEFACE=");
  serial_.print(store_.config().runtime.activeFaceId);
  serial_.print(" EFFECT=");
  serial_.println(effectModeName(store_.config().runtime.effect));
}

void SerialProtocol::sendMap() const {
  serial_.print("MAP ");
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    serial_.print(store_.config().mapping.physToLogical[index]);
    if (index + 1U < kNumPixels) {
      serial_.print(',');
    }
  }
  serial_.println();
}

void SerialProtocol::sendRtc() const {
  DynamicJsonDocument doc(512);
  JsonObject root = doc.to<JsonObject>();
  runtime_.writeRtcJson(root, millis());
  if (!(root["present"] | false)) {
    serial_.println("RTC STATUS MISSING");
    return;
  }
  serial_.print("RTC ");
  serial_.print(static_cast<const char*>(root["summary"] | ""));
  serial_.print(" LOST_POWER=");
  serial_.println((root["lostPower"] | false) ? 1 : 0);
}

bool SerialProtocol::parseRtc(const String& payload, DateTime& out) const {
  int values[6] = {0, 0, 0, 0, 0, 0};
  size_t start = 0;
  for (uint8_t index = 0; index < 6; ++index) {
    const int split = payload.indexOf(' ', start);
    const String token = split < 0 ? payload.substring(start) : payload.substring(start, split);
    if (token.length() == 0) {
      return false;
    }
    values[index] = token.toInt();
    start = split < 0 ? payload.length() : static_cast<size_t>(split + 1);
  }
  out = DateTime(values[0], values[1], values[2], values[3], values[4], values[5]);
  return true;
}

bool SerialProtocol::parseColor(const String& payload, String& target, RgbColor& color) const {
  const int firstSpace = payload.indexOf(' ');
  if (firstSpace < 0) {
    return false;
  }
  target = payload.substring(0, firstSpace);
  size_t start = static_cast<size_t>(firstSpace + 1);
  int values[3] = {0, 0, 0};
  for (uint8_t index = 0; index < 3; ++index) {
    const int split = payload.indexOf(' ', start);
    const String token = split < 0 ? payload.substring(start) : payload.substring(start, split);
    if (token.length() == 0) {
      return false;
    }
    values[index] = static_cast<int>(constrain(token.toInt(), 0L, 255L));
    start = split < 0 ? payload.length() : static_cast<size_t>(split + 1);
  }
  color = makeRgbColor(values[0], values[1], values[2]);
  return true;
}

int SerialProtocol::resolvePresetIndex(const String& token) const {
  for (size_t index = 0; index < store_.config().presetCount; ++index) {
    const PresetDefinition& preset = store_.config().presets[index];
    if (preset.id.equalsIgnoreCase(token) || preset.name.equalsIgnoreCase(token)) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

void SerialProtocol::handleLine(const String& rawLine) {
  String line = rawLine;
  line.trim();
  if (line.length() == 0U) {
    return;
  }

  if (line == "HELLO" || line == "HELLO Clock/1") {
    sendHello("Clock/1");
    return;
  }

  if (line == "HELLO ClockCal/1") {
    sendHello("ClockCal/1");
    return;
  }

  if (line == "INFO?" || line == "GET INFO" || line == "CAL INFO?" || line == "INFO CAL?") {
    sendInfo();
    return;
  }

  if (line == "STATUS?" || line == "GET STATUS") {
    sendStatus();
    return;
  }

  if (line == "CUR?") {
    serial_.print("CUR ");
    serial_.println(runtime_.calibrationCursor());
    return;
  }

  if (line == "MAP?") {
    sendMap();
    return;
  }

  if (line == "VALIDATE?") {
    serial_.println(runtime_.validateMapping() ? "OK VALIDATE PASS" : "OK VALIDATE FAIL");
    return;
  }

  if (line == "MODE CAL") {
    runtime_.setCalibrationMode(true);
    replyOK("MODE CAL");
    return;
  }

  if (line == "MODE RUN") {
    runtime_.setCalibrationMode(false);
    replyOK("MODE RUN");
    return;
  }

  if (line == "NEXT") {
    runtime_.nextCalibration(+1);
    replyOK("NEXT");
    return;
  }

  if (line == "PREV") {
    runtime_.nextCalibration(-1);
    replyOK("PREV");
    return;
  }

  if (line.startsWith("SET ")) {
    if (!runtime_.setCalibrationCursor(static_cast<uint8_t>(line.substring(4).toInt()))) {
      replyERR("bad phys");
      return;
    }
    replyOK("SET");
    return;
  }

  if (line.startsWith("ASSIGN ")) {
    if (!runtime_.assignLogical(static_cast<uint8_t>(line.substring(7).toInt()))) {
      replyERR("bad logical");
      return;
    }
    replyOK("ASSIGN");
    return;
  }

  if (line == "UNASSIGN_CUR") {
    runtime_.unassignCurrent();
    replyOK("UNASSIGN_CUR");
    return;
  }

  if (line == "SAVE") {
    String error;
    if (!runtime_.saveConfigNow(&error)) {
      replyERR("SAVE");
      return;
    }
    replyOK("SAVE");
    return;
  }

  if (line == "LOAD") {
    String error;
    if (!runtime_.loadConfigNow(&error)) {
      replyERR("LOAD");
      return;
    }
    replyOK("LOAD");
    return;
  }

  if (line == "TEST DIGITS") {
    runtime_.testDigits();
    replyOK("TEST DIGITS");
    return;
  }

  if (line == "TEST SEGMENTS") {
    runtime_.testSegments();
    replyOK("TEST SEGMENTS");
    return;
  }

  if (line == "TEST ALL") {
    runtime_.testAll();
    replyOK("TEST ALL");
    return;
  }

  if (line == "MODE CLOCK") {
    runtime_.setMode(ClockMode::CLOCK);
    replyOK("MODE CLOCK");
    return;
  }

  if (line == "MODE CLOCK_SECONDS") {
    runtime_.setMode(ClockMode::CLOCK_SECONDS);
    replyOK("MODE CLOCK_SECONDS");
    return;
  }

  if (line == "MODE TIMER") {
    runtime_.setMode(ClockMode::TIMER);
    replyOK("MODE TIMER");
    return;
  }

  if (line == "MODE STOPWATCH") {
    runtime_.setMode(ClockMode::STOPWATCH);
    replyOK("MODE STOPWATCH");
    return;
  }

  if (line == "MODE COLOR_DEMO") {
    runtime_.setMode(ClockMode::COLOR_DEMO);
    replyOK("MODE COLOR_DEMO");
    return;
  }

  if (line == "HOURMODE 24H") {
    runtime_.setHourMode(HourMode::H24);
    replyOK("HOURMODE 24H");
    return;
  }

  if (line == "HOURMODE 12H") {
    runtime_.setHourMode(HourMode::H12);
    replyOK("HOURMODE 12H");
    return;
  }

  if (line == "SECONDSMODE BLINK") {
    runtime_.setSecondsMode(SecondsMode::BLINK);
    replyOK("SECONDSMODE BLINK");
    return;
  }

  if (line == "SECONDSMODE ON") {
    runtime_.setSecondsMode(SecondsMode::ON);
    replyOK("SECONDSMODE ON");
    return;
  }

  if (line == "SECONDSMODE OFF") {
    runtime_.setSecondsMode(SecondsMode::OFF);
    replyOK("SECONDSMODE OFF");
    return;
  }

  if (line == "TIMERFORMAT AUTO") {
    runtime_.setTimerFormat(TimerDisplayFormat::AUTO);
    replyOK("TIMERFORMAT AUTO");
    return;
  }

  if (line == "TIMERFORMAT HHMM") {
    runtime_.setTimerFormat(TimerDisplayFormat::HHMM);
    replyOK("TIMERFORMAT HHMM");
    return;
  }

  if (line == "TIMERFORMAT MMSS") {
    runtime_.setTimerFormat(TimerDisplayFormat::MMSS);
    replyOK("TIMERFORMAT MMSS");
    return;
  }

  if (line == "TIMERFORMAT SSCS") {
    runtime_.setTimerFormat(TimerDisplayFormat::SSCS);
    replyOK("TIMERFORMAT SSCS");
    return;
  }

  if (line.startsWith("BRIGHTNESS ")) {
    runtime_.setBrightness(
        static_cast<uint8_t>(constrain(line.substring(11).toInt(), 1L, 255L)));
    replyOK(String("BRIGHTNESS ") + store_.config().runtime.brightness);
    return;
  }

  if (line.startsWith("COLONBRIGHTNESS ")) {
    runtime_.setColonBrightness(
        static_cast<uint8_t>(constrain(line.substring(16).toInt(), 0L, 255L)));
    replyOK(String("COLONBRIGHTNESS ") + store_.config().runtime.colonBrightness);
    return;
  }

  if (line.startsWith("COLOR ")) {
    String target;
    RgbColor color = makeRgbColor(0, 0, 0);
    if (!parseColor(line.substring(6), target, color) || !runtime_.setColorTarget(target, color)) {
      replyERR("COLOR");
      return;
    }
    replyOK(String("COLOR ") + target);
    return;
  }

  if (line.startsWith("PRESET ")) {
    const int presetIndex = resolvePresetIndex(line.substring(7));
    if (presetIndex < 0 || !runtime_.setActivePreset(store_.config().presets[presetIndex].id)) {
      replyERR("PRESET");
      return;
    }
    replyOK(String("PRESET ") + store_.config().presets[presetIndex].id);
    return;
  }

  if (line.startsWith("FACE ")) {
    if (!runtime_.setActiveFace(line.substring(5))) {
      replyERR("FACE");
      return;
    }
    replyOK(String("FACE ") + store_.config().runtime.activeFaceId);
    return;
  }

  if (line.startsWith("EFFECT ")) {
    runtime_.setEffect(effectModeFromString(line.substring(7)));
    replyOK(String("EFFECT ") + effectModeName(store_.config().runtime.effect));
    return;
  }

  if (line == "RTC READ") {
    sendRtc();
    return;
  }

  if (line.startsWith("RTC SET ")) {
    DateTime value;
    if (!parseRtc(line.substring(8), value) || !runtime_.setRtc(value)) {
      replyERR("RTC");
      return;
    }
    replyOK("RTC SET");
    sendRtc();
    return;
  }

  if (line.startsWith("TIMER SET ")) {
    runtime_.setTimerPresetSeconds(line.substring(10).toInt());
    replyOK(String("TIMER SET ") + store_.config().runtime.timerPresetSeconds);
    return;
  }

  if (line == "TIMER START") {
    runtime_.startTimer(millis());
    replyOK("TIMER START");
    return;
  }

  if (line == "TIMER STOP") {
    runtime_.stopTimer(millis());
    replyOK("TIMER STOP");
    return;
  }

  if (line == "TIMER RESET") {
    runtime_.resetTimer();
    replyOK("TIMER RESET");
    return;
  }

  if (line == "STOPWATCH START") {
    runtime_.startStopwatch(millis());
    replyOK("STOPWATCH START");
    return;
  }

  if (line == "STOPWATCH STOP") {
    runtime_.stopStopwatch(millis());
    replyOK("STOPWATCH STOP");
    return;
  }

  if (line == "STOPWATCH RESET") {
    runtime_.resetStopwatch(millis());
    replyOK("STOPWATCH RESET");
    return;
  }

  replyERR(String("UNKNOWN ") + line);
}
