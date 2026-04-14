#include "HostRuntime.h"

#include "../../include/project_config.h"

#include <WiFi.h>
#include <stdio.h>
#include <time.h>

namespace {
DateTime fromTm(const tm& value) {
  return DateTime(value.tm_year + 1900, value.tm_mon + 1, value.tm_mday, value.tm_hour,
                  value.tm_min, value.tm_sec);
}
}

HostRuntime::HostRuntime(SettingsStore& store, LeonardoClient& renderer)
    : store_(store), renderer_(renderer) {}

void HostRuntime::begin() {
  timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  timerRunning_ = false;
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  syncFaceToMode();
  markDirty(false);
}

const ClockConfig& HostRuntime::config() const { return store_.config(); }

ClockConfig& HostRuntime::mutableConfig() { return store_.config(); }

void HostRuntime::markDirty(bool persist) {
  renderDirty_ = true;
  if (persist) {
    persistPending_ = true;
    lastSaveRequestMs_ = millis();
  }
}

void HostRuntime::syncFaceToMode() {
  const int activeIndex = findFaceIndex(config(), config().runtime.activeFaceId);
  if (activeIndex >= 0 && config().faces[activeIndex].baseMode == config().runtime.mode) {
    return;
  }
  for (size_t index = 0; index < config().faceCount; ++index) {
    if (config().faces[index].baseMode == config().runtime.mode) {
      mutableConfig().runtime.activeFaceId = config().faces[index].id;
      if (config().faces[index].presetId.length() > 0) {
        mutableConfig().runtime.activePresetId = config().faces[index].presetId;
      }
      return;
    }
  }
}

void HostRuntime::reloadFromConfig() {
  timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  timerRunning_ = false;
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  ntpConfigured_ = false;
  ntpValid_ = false;
  lastStatusPollMs_ = 0;
  lastRtcPollMs_ = 0;
  lastMapPollMs_ = 0;
  lastControlRefreshMs_ = 0;
  syncFaceToMode();
  markDirty(false);
}

bool HostRuntime::saveConfigNow(String* error) {
  persistPending_ = false;
  return store_.save(error);
}

bool HostRuntime::loadConfigNow(String* error) {
  if (!store_.load(error)) {
    return false;
  }
  reloadFromConfig();
  return true;
}

void HostRuntime::setMode(ClockMode mode) {
  mutableConfig().runtime.mode = mode;
  syncFaceToMode();
  markDirty();
}

void HostRuntime::setHourMode(HourMode mode) {
  mutableConfig().runtime.hourMode = mode;
  markDirty();
}

void HostRuntime::setSecondsMode(SecondsMode mode) {
  mutableConfig().runtime.secondsMode = mode;
  markDirty();
}

void HostRuntime::setTimerFormat(TimerDisplayFormat format) {
  mutableConfig().runtime.timerFormat = format;
  markDirty();
}

void HostRuntime::setBrightness(uint8_t brightness) {
  mutableConfig().runtime.brightness = brightness;
  markDirty();
}

void HostRuntime::setColonBrightness(uint8_t brightness) {
  mutableConfig().runtime.colonBrightness = brightness;
  markDirty();
}

void HostRuntime::setEffect(EffectMode effect) {
  mutableConfig().runtime.effect = effect;
  markDirty();
}

bool HostRuntime::setActivePreset(const String& id) {
  if (findPresetIndex(config(), id) < 0) {
    return false;
  }
  mutableConfig().runtime.activePresetId = id;
  markDirty();
  return true;
}

bool HostRuntime::setActiveFace(const String& id) {
  if (findFaceIndex(config(), id) < 0) {
    return false;
  }
  mutableConfig().runtime.activeFaceId = id;
  markDirty();
  return true;
}

bool HostRuntime::setColorTarget(const String& target, const RgbColor& color) {
  const int presetIndex = findPresetIndex(config(), config().runtime.activePresetId);
  if (presetIndex < 0) {
    return false;
  }
  PresetDefinition& preset = mutableConfig().presets[presetIndex];
  if (target == "CLOCK") {
    preset.palette.digit1 = color;
    preset.palette.digit2 = color;
    preset.palette.digit3 = color;
    preset.palette.digit4 = color;
  } else if (target == "HOURS") {
    preset.palette.digit1 = color;
    preset.palette.digit2 = color;
  } else if (target == "MINUTES") {
    preset.palette.digit3 = color;
    preset.palette.digit4 = color;
  } else if (target == "DIGIT1") {
    preset.palette.digit1 = color;
  } else if (target == "DIGIT2") {
    preset.palette.digit2 = color;
  } else if (target == "DIGIT3") {
    preset.palette.digit3 = color;
  } else if (target == "DIGIT4") {
    preset.palette.digit4 = color;
  } else if (target == "ACCENT") {
    preset.palette.accent = color;
  } else if (target == "SECONDS") {
    preset.palette.seconds = color;
  } else if (target == "TIMER") {
    preset.palette.timerHours = color;
    preset.palette.timerMinutes = color;
    preset.palette.timerSeconds = color;
  } else if (target == "TIMER_HOURS") {
    preset.palette.timerHours = color;
  } else if (target == "TIMER_MINUTES") {
    preset.palette.timerMinutes = color;
  } else if (target == "TIMER_SECONDS") {
    preset.palette.timerSeconds = color;
  } else if (target == "STOPWATCH") {
    preset.palette.stopwatchHours = color;
    preset.palette.stopwatchMinutes = color;
    preset.palette.stopwatchSeconds = color;
  } else if (target == "STOPWATCH_HOURS") {
    preset.palette.stopwatchHours = color;
  } else if (target == "STOPWATCH_MINUTES") {
    preset.palette.stopwatchMinutes = color;
  } else if (target == "STOPWATCH_SECONDS") {
    preset.palette.stopwatchSeconds = color;
  } else {
    return false;
  }
  markDirty();
  return true;
}

void HostRuntime::setTimerPresetSeconds(uint32_t seconds) {
  mutableConfig().runtime.timerPresetSeconds = seconds;
  timerRemainingMs_ = seconds * 1000UL;
  timerRunning_ = false;
  markDirty();
}

void HostRuntime::startTimer(uint32_t nowMs) {
  setMode(ClockMode::TIMER);
  if (timerRemainingMs_ == 0U) {
    timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  }
  timerRunning_ = timerRemainingMs_ > 0U;
  timerStartedMs_ = nowMs;
  timerStartedRemainingMs_ = timerRemainingMs_;
  renderDirty_ = true;
}

void HostRuntime::stopTimer(uint32_t nowMs) {
  if (!timerRunning_) {
    return;
  }
  const uint32_t elapsed = nowMs - timerStartedMs_;
  timerRemainingMs_ =
      elapsed >= timerStartedRemainingMs_ ? 0U : (timerStartedRemainingMs_ - elapsed);
  timerRunning_ = false;
  renderDirty_ = true;
}

void HostRuntime::resetTimer() {
  timerRunning_ = false;
  timerRemainingMs_ = config().runtime.timerPresetSeconds * 1000UL;
  markDirty(false);
}

void HostRuntime::startStopwatch(uint32_t nowMs) {
  setMode(ClockMode::STOPWATCH);
  if (!stopwatchRunning_) {
    stopwatchRunning_ = true;
    stopwatchStartedMs_ = nowMs;
    renderDirty_ = true;
  }
}

void HostRuntime::stopStopwatch(uint32_t nowMs) {
  if (!stopwatchRunning_) {
    return;
  }
  stopwatchAccumulatedMs_ += nowMs - stopwatchStartedMs_;
  stopwatchRunning_ = false;
  renderDirty_ = true;
}

void HostRuntime::resetStopwatch(uint32_t nowMs) {
  stopwatchRunning_ = false;
  stopwatchAccumulatedMs_ = 0;
  stopwatchStartedMs_ = nowMs;
  renderDirty_ = true;
}

bool HostRuntime::setRtc(const DateTime& value) {
  renderer_.setRtc(value);
  lastRtcSyncMs_ = millis();
  renderDirty_ = true;
  return true;
}

void HostRuntime::requestRendererRefresh() {
  lastStatusPollMs_ = 0;
  lastRtcPollMs_ = 0;
  lastMapPollMs_ = 0;
}

void HostRuntime::controlRendererHost() {
  renderer_.controlHost();
  renderer_.setButtonReporting(config().rendererLink.buttonEvents);
}

void HostRuntime::controlRendererLocal() { renderer_.controlLocal(); }

void HostRuntime::runRendererTestSegments() { renderer_.testSegments(); }

void HostRuntime::runRendererTestDigits() { renderer_.testDigits(); }

void HostRuntime::runRendererTestAll() { renderer_.testAll(); }

DateTime HostRuntime::fallbackNow(uint32_t nowMs) const {
  const uint32_t totalSeconds = nowMs / 1000UL;
  const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
  const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60UL) % 60UL);
  const uint8_t hours = static_cast<uint8_t>((totalSeconds / 3600UL) % 24UL);
  return DateTime(2026, 1, 1, hours, minutes, seconds);
}

uint32_t HostRuntime::stopwatchElapsedMs(uint32_t nowMs) const {
  if (!stopwatchRunning_) {
    return stopwatchAccumulatedMs_;
  }
  return stopwatchAccumulatedMs_ + (nowMs - stopwatchStartedMs_);
}

void HostRuntime::updateTimerState(uint32_t nowMs) {
  if (timerRunning_) {
    const uint32_t elapsed = nowMs - timerStartedMs_;
    if (elapsed >= timerStartedRemainingMs_) {
      timerRunning_ = false;
      timerRemainingMs_ = 0;
      renderDirty_ = true;
    } else {
      const uint32_t newRemaining = timerStartedRemainingMs_ - elapsed;
      if (newRemaining != timerRemainingMs_) {
        timerRemainingMs_ = newRemaining;
        renderDirty_ = true;
      }
    }
  }
}

void HostRuntime::handleRendererEvents(uint32_t nowMs) {
  String buttonId;
  while (renderer_.consumeButtonEvent(buttonId)) {
    if (buttonId == "BTN1") {
      const uint8_t next = (static_cast<uint8_t>(config().runtime.mode) + 1U) % 5U;
      setMode(static_cast<ClockMode>(next));
    } else if (buttonId == "BTN2") {
      if (config().runtime.mode == ClockMode::TIMER) {
        setTimerPresetSeconds(config().runtime.timerPresetSeconds + 60UL);
      } else if (config().runtime.mode == ClockMode::STOPWATCH) {
        if (stopwatchRunning_) {
          stopStopwatch(nowMs);
        } else {
          startStopwatch(nowMs);
        }
      } else {
        setMode(ClockMode::CLOCK);
      }
    } else if (buttonId == "BTN3") {
      if (config().runtime.mode == ClockMode::TIMER) {
        const uint32_t seconds = config().runtime.timerPresetSeconds >= 60UL
                                     ? (config().runtime.timerPresetSeconds - 60UL)
                                     : 0UL;
        setTimerPresetSeconds(seconds);
      } else if (config().runtime.mode == ClockMode::STOPWATCH) {
        resetStopwatch(nowMs);
      } else {
        setMode(ClockMode::COLOR_DEMO);
      }
    }
  }

  if (renderer_.consumeTimerDoneEvent()) {
    timerRunning_ = false;
    timerRemainingMs_ = 0;
    renderDirty_ = true;
  }

  if (renderer_.consumeHostTimeoutEvent()) {
    lastControlRefreshMs_ = 0;
    renderDirty_ = true;
  }
}

void HostRuntime::maintainRendererLink(uint32_t nowMs) {
  if (!renderer_.isOnline(nowMs)) {
    if ((nowMs - lastHelloRequestMs_) >= ProjectConfig::kRendererHelloRetryMs) {
      renderer_.requestHello();
      lastHelloRequestMs_ = nowMs;
    }
    return;
  }

  if ((nowMs - lastControlRefreshMs_) >= ProjectConfig::kRendererControlRefreshMs) {
    renderer_.controlHost();
    renderer_.setButtonReporting(config().rendererLink.buttonEvents);
    if (!renderer_.caps().seen) {
      renderer_.requestCaps();
    }
    lastControlRefreshMs_ = nowMs;
  }

  if ((nowMs - lastStatusPollMs_) >= ProjectConfig::kStatusPollMs) {
    renderer_.requestStatus();
    lastStatusPollMs_ = nowMs;
  }

  if ((nowMs - lastRtcPollMs_) >= ProjectConfig::kRendererRtcPollMs) {
    renderer_.requestRtc();
    lastRtcPollMs_ = nowMs;
  }

  if ((nowMs - lastMapPollMs_) >= ProjectConfig::kRendererMapPollMs) {
    renderer_.requestMap();
    lastMapPollMs_ = nowMs;
  }
}

void HostRuntime::updateTimeSource(uint32_t nowMs) {
  if (!ntpConfigured_ && config().time.ntpEnabled) {
    configTzTime(config().time.timezone.c_str(), config().time.ntpPrimary.c_str(),
                 config().time.ntpSecondary.c_str());
    ntpConfigured_ = true;
  }

  if (config().time.ntpEnabled && (nowMs - lastNtpPollMs_) >= ProjectConfig::kNtpPollMs) {
    lastNtpPollMs_ = nowMs;
    tm tmInfo = {};
    if (getLocalTime(&tmInfo, 10)) {
      ntpValid_ = true;
      timeSource_ = TimeSource::NTP;
      if (renderer_.isOnline(nowMs) &&
          (nowMs - lastRtcSyncMs_) >= (config().time.rtcSyncIntervalSeconds * 1000UL)) {
        renderer_.setRtc(fromTm(tmInfo));
        lastRtcSyncMs_ = nowMs;
      }
    }
  }

  if (!ntpValid_) {
    timeSource_ = (renderer_.rtc().present && renderer_.rtc().parsed) ? TimeSource::RENDERER_RTC
                                                                      : TimeSource::FALLBACK;
  }
}

DateTime HostRuntime::currentTime(uint32_t nowMs) const {
  if (timeSource_ == TimeSource::NTP) {
    time_t now = time(nullptr);
    tm local = {};
    localtime_r(&now, &local);
    return fromTm(local);
  }

  if (timeSource_ == TimeSource::RENDERER_RTC && renderer_.rtc().parsed) {
    const uint32_t deltaSeconds = (nowMs - renderer_.rtc().sampledAtMs) / 1000UL;
    return DateTime(renderer_.rtc().dateTime.unixtime() + deltaSeconds);
  }

  return fallbackNow(nowMs);
}

const char* HostRuntime::timeSourceName() const {
  switch (timeSource_) {
    case TimeSource::NTP:
      return "ntp";
    case TimeSource::RENDERER_RTC:
      return "renderer_rtc";
    default:
      return "fallback";
  }
}

void HostRuntime::renderIfNeeded(uint32_t nowMs) {
  if (!renderer_.isOnline(nowMs)) {
    return;
  }

  if (!renderDirty_ && (nowMs - lastRenderMs_) < ProjectConfig::kRenderRefreshMs) {
    return;
  }

  CompileContext context;
  context.nowMs = nowMs;
  context.now = currentTime(nowMs);
  context.timerRemainingMs = timerRemainingMs_;
  context.timerRunning = timerRunning_;
  context.stopwatchElapsedMs = stopwatchElapsedMs(nowMs);

  SegmentFrame frame;
  compiler_.compile(config(), context, frame);
  renderer_.sendFrame24(frame);
  lastRenderMs_ = nowMs;
  renderDirty_ = false;
}

void HostRuntime::update(uint32_t nowMs) {
  renderer_.poll(nowMs);
  handleRendererEvents(nowMs);
  maintainRendererLink(nowMs);
  updateTimeSource(nowMs);
  updateTimerState(nowMs);

  if (persistPending_ && (nowMs - lastSaveRequestMs_) >= ProjectConfig::kSaveDebounceMs) {
    saveConfigNow(nullptr);
  }

  renderIfNeeded(nowMs);
}

void HostRuntime::writeInfoJson(JsonObject root) const {
  root["board"] = "ESP32-WROOM-32E";
  root["role"] = "host";
  root["name"] = CLOCK_DEVICE_NAME;
  root["protocol"] = "ClockHost/1";
  root["rendererProtocol"] = "ClockRender/1";
  JsonObject link = root.createNestedObject("rendererLink");
  link["baud"] = config().rendererLink.baud;
  link["txPin"] = config().rendererLink.txPin;
  link["rxPin"] = config().rendererLink.rxPin;
}

void HostRuntime::writeStatusJson(JsonObject root, uint32_t nowMs) const {
  root["mode"] = clockModeName(config().runtime.mode);
  root["hourMode"] = hourModeName(config().runtime.hourMode);
  root["secondsMode"] = secondsModeName(config().runtime.secondsMode);
  root["timerFormat"] = timerFormatName(config().runtime.timerFormat);
  root["effect"] = effectModeName(config().runtime.effect);
  root["brightness"] = config().runtime.brightness;
  root["colonBrightness"] = config().runtime.colonBrightness;
  root["timerPresetSeconds"] = config().runtime.timerPresetSeconds;
  root["timerRemainingSeconds"] = (timerRemainingMs_ + 999UL) / 1000UL;
  root["timerRunning"] = timerRunning_;
  root["stopwatchRunning"] = stopwatchRunning_;
  root["stopwatchElapsedMs"] = stopwatchElapsedMs(nowMs);
  root["activePresetId"] = config().runtime.activePresetId;
  root["activeFaceId"] = config().runtime.activeFaceId;
  root["rendererOnline"] = renderer_.isOnline(nowMs);
}

void HostRuntime::writeRtcJson(JsonObject root, uint32_t nowMs) const {
  const DateTime now = currentTime(nowMs);
  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u", now.year(), now.month(),
           now.day(), now.hour(), now.minute(), now.second());
  root["present"] = true;
  root["summary"] = buffer;
  root["source"] = timeSourceName();
  root["rendererRtcPresent"] = renderer_.rtc().present;
  root["rendererRtcLostPower"] = renderer_.rtc().lostPower;
}

void HostRuntime::writeRendererJson(JsonObject root, uint32_t nowMs) const {
  root["connected"] = renderer_.isOnline(nowMs);
  root["lastSeenMs"] = renderer_.lastSeenMs();
  root["buttonEvents"] = config().rendererLink.buttonEvents;

  JsonObject hello = root.createNestedObject("hello");
  hello["seen"] = renderer_.hello().seen;
  hello["protocol"] = renderer_.hello().protocol;
  hello["canonicalProtocol"] = renderer_.hello().canonicalProtocol;
  hello["name"] = renderer_.hello().name;
  hello["firmwareVersion"] = renderer_.hello().firmwareVersion;
  hello["pixels"] = renderer_.hello().pixels;
  hello["logical"] = renderer_.hello().logical;

  JsonObject status = root.createNestedObject("status");
  status["seen"] = renderer_.status().seen;
  status["mode"] = renderer_.status().mode;
  status["control"] = renderer_.status().control;
  status["frame"] = renderer_.status().frame;
  status["buttonReporting"] = renderer_.status().buttonReporting;
  status["mapValid"] = renderer_.status().mapValid;
  status["brightness"] = renderer_.status().brightness;
  status["colonBrightness"] = renderer_.status().colonBrightness;
  status["timerRemainingSeconds"] = renderer_.status().timerRemainingSeconds;

  JsonObject rtc = root.createNestedObject("rtc");
  rtc["seen"] = renderer_.rtc().seen;
  rtc["present"] = renderer_.rtc().present;
  rtc["lostPower"] = renderer_.rtc().lostPower;
  rtc["summary"] = renderer_.rtc().summary;

  JsonArray mapping = root.createNestedArray("mapping");
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    mapping.add(renderer_.mapping().values[index]);
  }
}

String HostRuntime::exportRendererMappingJson() const {
  DynamicJsonDocument doc(8192);
  JsonObject root = doc.to<JsonObject>();
  root["schema"] = "clock-renderer-map/v1";
  root["protocol"] = renderer_.hello().canonicalProtocol.length() > 0
                         ? renderer_.hello().canonicalProtocol
                         : "ClockRender/1";
  JsonArray mapping = root.createNestedArray("mapping");
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    JsonObject item = mapping.createNestedObject();
    item["physical"] = index;
    item["logical"] = renderer_.mapping().values[index];
    item["name"] = logicalName(renderer_.mapping().values[index]);
  }
  String output;
  serializeJsonPretty(doc, output);
  return output;
}

String HostRuntime::exportRendererMapHeader() const {
  String output;
  output += "#pragma once\n#include <stdint.h>\n\n";
  output += "constexpr uint8_t kPhysToLogical[31] = {\n  ";
  for (uint8_t index = 0; index < kNumPixels; ++index) {
    output += String(renderer_.mapping().values[index]);
    if (index + 1U < kNumPixels) {
      output += ", ";
    }
    if ((index % 8U) == 7U && index + 1U < kNumPixels) {
      output += "\n  ";
    }
  }
  output += "\n};\n";
  return output;
}
