#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <Wire.h>
#include <avr/power.h>

// Boards flashed through the chip's factory USB (DFU) bootloader keep the
// factory CKDIV8 fuse, which boots the CPU at 16 MHz / 8. Arduino's main()
// calls initVariant() before USB starts, so undo the divider there.
// Harmless on boards with the Arduino (Caterina) bootloader.
void initVariant() { clock_prescale_set(clock_div_1); }

#include "src/calibration/ClockProtocol.h"
#include "src/display/LedMap.h"
#include "src/display/LogicalDisplay.h"
#include "src/hal/Buttons.h"
#include "src/hal/PixelDriver_NeoPixel.h"
#include "src/hal/RotaryEncoder.h"
#include "src/hal/Rtc_DS3231.h"
#include "src/settings/SettingsStore.h"
#include "src/ui/AppStateMachine.h"

namespace Pins {
static constexpr uint8_t kPixels = 5;
static constexpr uint8_t kBtn1 = 8;
static constexpr uint8_t kBtn2 = 9;
static constexpr uint8_t kBtn3 = 10;
// Rotary encoder (KY-040) on the bottom header J2: CLK = J2 pin 5, DT = J2 pin 4, SW = J2 pin 6.
static constexpr uint8_t kEncClk = 0;  // D0 / PD2 / INT2
static constexpr uint8_t kEncDt = 1;   // D1 / PD3 / INT3
static constexpr uint8_t kEncSw = 11;  // D11 / PB7
}

namespace ProjectInfo {
static constexpr const char* kFirmwareName = "clock_firmware";
static constexpr const char* kFirmwareVersion = "0.4.0";
}

PixelDriver_NeoPixel gPixels(Pins::kPixels, kNumPixels);
LedMap gMap;
LogicalDisplay gDisplay(gPixels, gMap);
Buttons gButtons(Pins::kBtn1, Pins::kBtn2, Pins::kBtn3);
RotaryEncoder gKnob(Pins::kEncClk, Pins::kEncDt, Pins::kEncSw);
SettingsStore gSettings;
AppStateMachine gApp;
Rtc_DS3231 gRtc;
ClockProtocol gProtocol(Serial, gSettings, gApp, gRtc, ProjectInfo::kFirmwareName,
                        ProjectInfo::kFirmwareVersion);

static DateTime fallbackNow(uint32_t nowMs) {
  const uint32_t totalSeconds = nowMs / 1000UL;
  const uint8_t seconds = static_cast<uint8_t>(totalSeconds % 60UL);
  const uint8_t minutes = static_cast<uint8_t>((totalSeconds / 60UL) % 60UL);
  const uint8_t hours = static_cast<uint8_t>((totalSeconds / 3600UL) % 24UL);
  return DateTime(2026, 1, 1, hours, minutes, seconds);
}

// ---------------------------------------------------------------------------
// Knob behaviour
//   Turn:        brightness (16 steps). In TIMER mode, while the timer is not
//                running, turning sets the timer length instead.
//   Night red:   at the lowest step, stop turning for kNightPauseMs, then turn
//                down one more click -> every lit segment goes pure red at level 1.
//                Turn up one click to leave it (back to the lowest normal step).
//   Click:       CLOCK -> TIMER -> STOPWATCH -> CLOCK
//   Hold ~1 s:   (release between 0.6 and 2 s) TIMER/STOPWATCH: start / pause.
//                CLOCK: toggle night red.
//   Hold 2 s:    TIMER: reset to the set length. STOPWATCH: reset to 0.
// ---------------------------------------------------------------------------
namespace Knob {
static const uint8_t kLevels[] = {6, 8, 11, 16, 22, 30, 40, 52, 64, 80, 100, 128, 160, 192, 224, 255};
static constexpr uint8_t kLevelCount = sizeof(kLevels) / sizeof(kLevels[0]);
static constexpr uint32_t kNightPauseMs = 700UL;
static constexpr uint8_t kNightRedLevel = 1;
static constexpr uint32_t kTimerMaxSeconds = 12UL * 3600UL;
static constexpr int kNightEepromAddr = 100;  // free byte between the LED map (0..38) and settings (128..)
static constexpr uint8_t kNightOnMagic = 0xA5;
bool night = false;
uint32_t lastTurnMs = 0;
}  // namespace Knob

static void setNight(bool on) {
  Knob::night = on;
  EEPROM.update(Knob::kNightEepromAddr, on ? Knob::kNightOnMagic : 0x00);
}

static uint8_t nearestLevelIndex(uint8_t brightness) {
  uint8_t best = 0;
  for (uint8_t i = 1; i < Knob::kLevelCount; ++i) {
    const int dBest = abs(static_cast<int>(Knob::kLevels[best]) - brightness);
    const int dI = abs(static_cast<int>(Knob::kLevels[i]) - brightness);
    if (dI < dBest) best = i;
  }
  return best;
}

static void turnBrightness(int8_t detents, uint32_t nowMs) {
  const bool pausedFirst = (nowMs - Knob::lastTurnMs) >= Knob::kNightPauseMs;
  Knob::lastTurnMs = nowMs;

  if (Knob::night) {
    if (detents > 0) {
      setNight(false);
      gSettings.setBrightness(Knob::kLevels[0]);
    }
    return;
  }

  int idx = nearestLevelIndex(gSettings.settings().brightness);
  if (detents < 0 && idx == 0) {
    // Already at the bottom: only a fresh turn (after a pause) drops into night red.
    if (pausedFirst) setNight(true);
    return;
  }
  idx += detents;
  if (idx < 0) idx = 0;  // stop at the lowest step; a pause is needed before night red
  if (idx >= Knob::kLevelCount) idx = Knob::kLevelCount - 1;
  gSettings.setBrightness(Knob::kLevels[idx]);
}

static uint32_t timerStep(uint32_t s, bool up) {
  const uint32_t t = up ? s : (s == 0 ? 0 : s - 1);
  if (t < 60UL) return 5UL;      // under 1 min: 5 s steps
  if (t < 600UL) return 30UL;    // 1-10 min: 30 s
  if (t < 3600UL) return 60UL;   // 10-60 min: 1 min
  return 300UL;                  // over 1 h: 5 min
}

static void turnTimer(int8_t detents) {
  uint32_t s = gSettings.settings().timerPresetSeconds;
  while (detents > 0) {
    const uint32_t step = timerStep(s, true);
    s = (s / step + 1UL) * step;
    if (s > Knob::kTimerMaxSeconds) s = Knob::kTimerMaxSeconds;
    --detents;
  }
  while (detents < 0 && s > 0) {
    const uint32_t step = timerStep(s, false);
    s = ((s - 1UL) / step) * step;
    ++detents;
  }
  gSettings.setTimerPresetSeconds(s);
  gApp.resetTimer(s);
}

static void renderIfRunning(uint32_t nowMs) {
  const DateTime now = gRtc.isPresent() ? gRtc.now() : fallbackNow(nowMs);
  if (Knob::night) {
    // Render at full scale so no segment rounds to off, then the driver turns
    // every lit pixel into red at kNightRedLevel.
    ClockSettings s = gSettings.settings();
    s.brightness = 255;
    s.colonBrightness = s.colonBrightness == 0 ? 0 : 255;
    gPixels.setNightRed(Knob::kNightRedLevel);
    gApp.render(gDisplay, gPixels, s, now, nowMs);
    gPixels.setNightRed(0);
    return;
  }
  gApp.render(gDisplay, gPixels, gSettings.settings(), now, nowMs);
}

static void setModeAndSave(ClockMode mode) {
  gApp.setMode(mode);
  gSettings.setMode(mode);
}

static void handleKnob(uint32_t nowMs) {
  gKnob.update();
  bool changed = false;

  const int8_t detents = gKnob.takeDetents();
  if (detents != 0) {
    if (gApp.mode() == ClockMode::TIMER && !gApp.timerRunning() && !Knob::night) {
      turnTimer(detents);
    } else {
      turnBrightness(detents, nowMs);
    }
    changed = true;
  }

  if (gKnob.takeClick()) {
    switch (gApp.mode()) {
      case ClockMode::TIMER:
        setModeAndSave(ClockMode::STOPWATCH);
        break;
      case ClockMode::STOPWATCH:
        setModeAndSave(ClockMode::CLOCK);
        break;
      default:
        setModeAndSave(ClockMode::TIMER);
        break;
    }
    changed = true;
  }

  if (gKnob.takeLongPress()) {
    if (gApp.mode() == ClockMode::TIMER) {
      if (gApp.timerRunning()) {
        gApp.stopTimer(nowMs);
      } else {
        gApp.startTimer(nowMs, gSettings.settings().timerPresetSeconds);
      }
    } else if (gApp.mode() == ClockMode::STOPWATCH) {
      gApp.toggleStopwatch(nowMs);
    } else {
      setNight(!Knob::night);
    }
    changed = true;
  }

  if (gKnob.takeHold()) {
    if (gApp.mode() == ClockMode::TIMER) {
      gApp.resetTimer(gSettings.settings().timerPresetSeconds);
    } else if (gApp.mode() == ClockMode::STOPWATCH) {
      gApp.resetStopwatch(nowMs);
    }
    changed = true;
  }

  if (changed) renderIfRunning(nowMs);
}

static void handleButtons(uint32_t nowMs) {
  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN1)) {
    gApp.cycleModeForward();
    gSettings.setMode(gApp.mode());
    renderIfRunning(nowMs);
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN2)) {
    if (gApp.mode() == ClockMode::TIMER) {
      gSettings.setTimerPresetSeconds(gSettings.settings().timerPresetSeconds + 60UL);
      gApp.resetTimer(gSettings.settings().timerPresetSeconds);
      renderIfRunning(nowMs);
    } else if (gApp.mode() == ClockMode::STOPWATCH) {
      gApp.toggleStopwatch(nowMs);
      renderIfRunning(nowMs);
    } else {
      gApp.setMode(ClockMode::CLOCK);
      gSettings.setMode(gApp.mode());
      renderIfRunning(nowMs);
    }
  }

  if (gButtons.wasShortPressed(Buttons::ButtonId::BTN3)) {
    if (gApp.mode() == ClockMode::TIMER) {
      uint32_t timerPresetSeconds = gSettings.settings().timerPresetSeconds;
      if (timerPresetSeconds >= 60UL) {
        timerPresetSeconds -= 60UL;
      } else {
        timerPresetSeconds = 0;
      }
      gSettings.setTimerPresetSeconds(timerPresetSeconds);
      gApp.resetTimer(gSettings.settings().timerPresetSeconds);
      renderIfRunning(nowMs);
    } else if (gApp.mode() == ClockMode::STOPWATCH) {
      gApp.resetStopwatch(nowMs);
      renderIfRunning(nowMs);
    } else {
      gApp.setMode(ClockMode::COLOR_DEMO);
      gSettings.setMode(gApp.mode());
      renderIfRunning(nowMs);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  gPixels.begin();
  gPixels.setBrightness(255);
  gButtons.begin();
  gKnob.begin();
  Knob::night = EEPROM.read(Knob::kNightEepromAddr) == Knob::kNightOnMagic;
  gMap.loadOrDefault();
  gSettings.begin();
  gApp.begin(gSettings.settings(), millis());
  gProtocol.begin(96U);

  if (gRtc.begin() && gRtc.lostPower()) {
    gRtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  renderIfRunning(millis());
  gProtocol.sendFirmwareInfo();
}

void loop() {
  const uint32_t nowMs = millis();

  gButtons.update();
  gProtocol.poll();
  handleButtons(nowMs);
  handleKnob(nowMs);

  if (gProtocol.consumeRenderRequested()) {
    renderIfRunning(nowMs);
  }

  if (gApp.update(nowMs)) {
    renderIfRunning(nowMs);
  }

  if (gApp.consumeTimerDoneEvent()) {
    gProtocol.sendTimerDone();
    renderIfRunning(nowMs);
  }

  gSettings.tick(nowMs);
}
