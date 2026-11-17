#include "WinderServer.h"

#include "../../include/project_config.h"

WinderServer::WinderServer(HostRuntime& runtime) : runtime_(runtime) {}

void WinderServer::begin(uint16_t port) {
  server_ = WiFiServer(port);
  server_.begin();
  server_.setNoDelay(true);
  started_ = true;
}

void WinderServer::update() {
  if (!started_) {
    return;
  }

  if (server_.hasClient()) {
    // One GUI at a time; a new connection replaces the old one.
    if (client_ && client_.connected()) {
      client_.stop();
    }
    client_ = server_.available();
    buffer_ = "";
  }

  if (!client_ || !client_.connected()) {
    return;
  }

  while (client_.available() > 0) {
    const char c = static_cast<char>(client_.read());
    if (c == '\n' || c == '\r') {
      buffer_.trim();
      if (buffer_.length() > 0) {
        handleLine(buffer_);
      }
      buffer_ = "";
    } else if (buffer_.length() < 256) {
      buffer_ += c;
    }
  }
}

void WinderServer::reply(const String& line) {
  if (client_ && client_.connected()) {
    client_.print(line);
    client_.print('\n');
  }
}

void WinderServer::sendHello(const String& alias) {
  const uint32_t nowMs = millis();
  String line = "OK HELLO " + alias;
  line += " NAME=" CLOCK_DEVICE_NAME;
  line += " FW=" + String(ProjectConfig::kFirmwareVersion);
  line += " PIXELS=31 LOGICAL=31";
  line += " RTC=" + String(runtime_.renderer().rtc().present ? 1 : 0);
  line += " CONTROL=HOST BTNREPORT=0 CAL=0 CANON=ClockRender/1";
  line += " RENDERER=" + String(runtime_.rendererOnline(nowMs) ? "online" : "offline");
  reply(line);
}

void WinderServer::sendInfo() {
  const uint32_t nowMs = millis();
  String line = "OK INFO BOARD=ESP32-host PIXELS=31 PROTO=ClockRender/1 TRANSPORT=WiFi";
  line += " RENDERER=" + String(runtime_.rendererOnline(nowMs) ? "online" : "offline");
  line += " RENDERER_FW=" + (runtime_.renderer().hello().firmwareVersion.length() > 0
                                 ? runtime_.renderer().hello().firmwareVersion
                                 : String("unknown"));
  line += " TIME=" + String(runtime_.timeSource());
  line += " IP=" + WiFi.localIP().toString();
  reply(line);
}

void WinderServer::sendStatus() {
  const uint32_t nowMs = millis();
  StaticJsonDocument<1024> doc;
  runtime_.writeStatusJson(doc.to<JsonObject>(), nowMs);
  String line = "OK STATUS MODE=" + String(static_cast<const char*>(doc["mode"]));
  line += " HOURMODE=" + String(static_cast<const char*>(doc["hourMode"]));
  line += " SECONDSMODE=" + String(static_cast<const char*>(doc["secondsMode"]));
  line += " BRIGHTNESS=" + String(static_cast<int>(doc["brightness"]));
  line += " COLONBRIGHTNESS=" + String(static_cast<int>(doc["colonBrightness"]));
  line += " RAINBOW=0";
  line += " TIMER_PRESET=" + String(static_cast<uint32_t>(doc["timerPresetSeconds"]));
  line += " TIMERFORMAT=" + String(static_cast<const char*>(doc["timerFormat"]));
  line += " TIMER_REMAINING=" + String(runtime_.timerRemainingSeconds());
  line += " TIMER_RUNNING=" + String(runtime_.timerRunning() ? 1 : 0);
  line += " STOPWATCH_RUNNING=" + String(runtime_.stopwatchRunning() ? 1 : 0);
  line += " CONTROL=HOST FRAME=FRAME24 BTNREPORT=0";
  line += " MAP_VALID=" + String(runtime_.renderer().status().mapValid ? 1 : 0);
  line += " CAL=0";
  reply(line);
}

void WinderServer::sendRtc() {
  const DateTime now = runtime_.now(millis());
  char buffer[48];
  snprintf(buffer, sizeof(buffer), "RTC %04u-%02u-%02u %02u:%02u:%02u LOST_POWER=%d", now.year(),
           now.month(), now.day(), now.hour(), now.minute(), now.second(),
           runtime_.renderer().rtc().lostPower ? 1 : 0);
  reply(buffer);
}

void WinderServer::sendMap() {
  String line = "MAP ";
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    if (index > 0) line += ",";
    line += String(runtime_.renderer().mapping().values[index]);
  }
  reply(line);
}

void WinderServer::handleLine(const String& rawLine) {
  String line = rawLine;
  line.trim();
  String upper = line;
  upper.toUpperCase();
  const uint32_t nowMs = millis();

  if (upper == "PING") {
    reply("OK PONG");
  } else if (upper == "HELLO") {
    sendHello("ClockRender/1");
  } else if (upper.startsWith("HELLO ")) {
    sendHello(line.substring(6));
  } else if (upper == "INFO?") {
    sendInfo();
  } else if (upper == "STATUS?") {
    sendStatus();
  } else if (upper == "MAP?") {
    sendMap();
  } else if (upper == "VALIDATE?") {
    reply(String("OK VALIDATE ") + (runtime_.renderer().status().mapValid ? "1" : "0"));
  } else if (upper == "RTC READ") {
    sendRtc();
  } else if (upper.startsWith("RTC SET ")) {
    int y, mo, d, h, mi, s;
    if (sscanf(line.c_str() + 8, "%d %d %d %d %d %d", &y, &mo, &d, &h, &mi, &s) == 6) {
      runtime_.setRtc(DateTime(y, mo, d, h, mi, s));
      reply("OK RTC SET");
    } else {
      reply("ERR RTC FORMAT");
    }
  } else if (upper == "MODE CAL" || upper == "MODE RUN" || upper == "CUR?" || upper == "PREV" ||
             upper == "NEXT" || upper.startsWith("SET ") || upper.startsWith("ASSIGN ") ||
             upper == "UNASSIGN_CUR" || upper == "SAVE" || upper == "LOAD") {
    reply("ERR CALIBRATION NEEDS USB (plug into the clock board and flash the calibration sketch)");
  } else if (upper.startsWith("MODE ")) {
    runtime_.setMode(clockModeFromString(upper.substring(5)));
    reply("OK MODE");
  } else if (upper.startsWith("HOURMODE ")) {
    runtime_.setHourMode(hourModeFromString(upper.substring(9)));
    reply("OK HOURMODE");
  } else if (upper.startsWith("SECONDSMODE ")) {
    runtime_.setSecondsMode(secondsModeFromString(upper.substring(12)));
    reply("OK SECONDSMODE");
  } else if (upper.startsWith("TIMERFORMAT ")) {
    runtime_.setTimerFormat(timerFormatFromString(upper.substring(12)));
    reply("OK TIMERFORMAT");
  } else if (upper.startsWith("BRIGHTNESS ")) {
    runtime_.setBrightness(static_cast<uint8_t>(constrain(line.substring(11).toInt(), 0, 255)));
    reply("OK BRIGHTNESS");
  } else if (upper.startsWith("COLONBRIGHTNESS ")) {
    runtime_.setColonBrightness(static_cast<uint8_t>(constrain(line.substring(16).toInt(), 0, 255)));
    reply("OK COLONBRIGHTNESS");
  } else if (upper.startsWith("NIGHTRED ")) {
    runtime_.setNightRed(line.substring(9).toInt() != 0);
    reply("OK NIGHTRED");
  } else if (upper.startsWith("COLOR ")) {
    char target[24];
    int r, g, b;
    if (sscanf(upper.c_str() + 6, "%23s %d %d %d", target, &r, &g, &b) == 4 &&
        runtime_.setColorTarget(String(target),
                                makeRgbColor(constrain(r, 0, 255), constrain(g, 0, 255),
                                             constrain(b, 0, 255)))) {
      reply("OK COLOR");
    } else {
      reply("ERR COLOR");
    }
  } else if (upper.startsWith("PRESET ")) {
    const String id = line.substring(7);
    reply(runtime_.setActivePreset(id) ? "OK PRESET" : "ERR PRESET UNKNOWN");
  } else if (upper.startsWith("TIMER SET ")) {
    runtime_.setTimerPresetSeconds(static_cast<uint32_t>(max(0L, line.substring(10).toInt())));
    reply("OK TIMER SET");
  } else if (upper == "TIMER START") {
    runtime_.startTimer(nowMs);
    reply("OK TIMER START");
  } else if (upper == "TIMER STOP") {
    runtime_.stopTimer(nowMs);
    reply("OK TIMER STOP");
  } else if (upper == "TIMER RESET") {
    runtime_.resetTimer();
    reply("OK TIMER RESET");
  } else if (upper == "STOPWATCH START") {
    runtime_.startStopwatch(nowMs);
    reply("OK STOPWATCH START");
  } else if (upper == "STOPWATCH STOP") {
    runtime_.stopStopwatch(nowMs);
    reply("OK STOPWATCH STOP");
  } else if (upper == "STOPWATCH RESET") {
    runtime_.resetStopwatch(nowMs);
    reply("OK STOPWATCH RESET");
  } else if (upper == "TEST SEGMENTS") {
    runtime_.runRendererTestSegments();
    reply("OK TEST SEGMENTS");
  } else if (upper == "TEST DIGITS") {
    runtime_.runRendererTestDigits();
    reply("OK TEST DIGITS");
  } else if (upper == "TEST ALL") {
    runtime_.runRendererTestAll();
    reply("OK TEST ALL");
  } else {
    reply("ERR UNKNOWN " + line);
  }
}
