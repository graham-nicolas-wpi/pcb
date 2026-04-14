#include "RendererRuntime.h"

#include "../render/SegmentFrame.h"
#include "../render/SegmentFrameRenderer.h"
#include "../settings/SettingsStore.h"
#include "../ui/AppStateMachine.h"

RendererRuntime::RendererRuntime(Buttons& buttons, SettingsStore& settings, AppStateMachine& app,
                                 SegmentFrameRenderer& renderer)
    : buttons_(buttons),
      settings_(settings),
      app_(app),
      renderer_(renderer) {}

void RendererRuntime::begin(uint32_t nowMs) {
  controlMode_ = RendererControlMode::LOCAL;
  buttonReportingEnabled_ = false;
  hostFrameActive_ = false;
  renderRequested_ = true;
  lastHostActivityMs_ = nowMs;
  pendingButtonMask_ = 0;
  timerDoneLatched_ = false;
  hostTimeoutLatched_ = false;
}

void RendererRuntime::requestRender() { renderRequested_ = true; }

void RendererRuntime::noteHostActivity(uint32_t nowMs) {
  lastHostActivityMs_ = nowMs;
  hostTimeoutLatched_ = false;
}

void RendererRuntime::setControlMode(RendererControlMode mode, uint32_t nowMs) {
  controlMode_ = mode;
  noteHostActivity(nowMs);
  if (controlMode_ == RendererControlMode::LOCAL) {
    hostFrameActive_ = false;
  }
  requestRender();
}

RendererControlMode RendererRuntime::controlMode() const { return controlMode_; }

const char* RendererRuntime::controlModeName() const {
  return controlMode_ == RendererControlMode::HOST ? "HOST" : "LOCAL";
}

void RendererRuntime::useLocalRenderer(uint32_t nowMs) {
  hostFrameActive_ = false;
  noteHostActivity(nowMs);
  requestRender();
}

void RendererRuntime::setHostFrame(const SegmentFrame& frame, uint32_t nowMs) {
  hostFrame_ = frame;
  hostFrameActive_ = true;
  noteHostActivity(nowMs);
  requestRender();
}

bool RendererRuntime::hostFrameActive() const { return hostFrameActive_; }

void RendererRuntime::setButtonReporting(bool enabled) { buttonReportingEnabled_ = enabled; }

bool RendererRuntime::buttonReportingEnabled() const { return buttonReportingEnabled_; }

void RendererRuntime::queueButtonEvent(Buttons::ButtonId buttonId) {
  pendingButtonMask_ |= static_cast<uint8_t>(1U << static_cast<uint8_t>(buttonId));
}

bool RendererRuntime::consumeButtonEvent(ButtonEvent& event) {
  for (uint8_t index = 0; index < 3U; ++index) {
    const uint8_t mask = static_cast<uint8_t>(1U << index);
    if ((pendingButtonMask_ & mask) != 0U) {
      pendingButtonMask_ &= static_cast<uint8_t>(~mask);
      event.buttonId = static_cast<Buttons::ButtonId>(index);
      return true;
    }
  }
  return false;
}

bool RendererRuntime::consumeTimerDoneEvent() {
  if (!timerDoneLatched_) {
    return false;
  }
  timerDoneLatched_ = false;
  return true;
}

bool RendererRuntime::consumeHostTimeoutEvent() {
  if (!hostTimeoutLatched_) {
    return false;
  }
  hostTimeoutLatched_ = false;
  return true;
}

void RendererRuntime::handleButtonPress(Buttons::ButtonId buttonId, uint32_t nowMs) {
  if (controlMode_ == RendererControlMode::HOST) {
    if (buttonReportingEnabled_) {
      queueButtonEvent(buttonId);
    }
    return;
  }

  switch (buttonId) {
    case Buttons::ButtonId::BTN1:
      app_.cycleModeForward();
      settings_.setMode(app_.mode());
      requestRender();
      return;

    case Buttons::ButtonId::BTN2:
      if (app_.mode() == ClockMode::TIMER) {
        settings_.setTimerPresetSeconds(settings_.settings().timerPresetSeconds + 60UL);
        app_.resetTimer(settings_.settings().timerPresetSeconds);
      } else if (app_.mode() == ClockMode::STOPWATCH) {
        app_.toggleStopwatch(nowMs);
      } else {
        app_.setMode(ClockMode::CLOCK);
        settings_.setMode(app_.mode());
      }
      requestRender();
      return;

    case Buttons::ButtonId::BTN3:
      if (app_.mode() == ClockMode::TIMER) {
        uint32_t timerPresetSeconds = settings_.settings().timerPresetSeconds;
        timerPresetSeconds = timerPresetSeconds >= 60UL ? (timerPresetSeconds - 60UL) : 0UL;
        settings_.setTimerPresetSeconds(timerPresetSeconds);
        app_.resetTimer(settings_.settings().timerPresetSeconds);
      } else if (app_.mode() == ClockMode::STOPWATCH) {
        app_.resetStopwatch(nowMs);
      } else {
        app_.setMode(ClockMode::COLOR_DEMO);
        settings_.setMode(app_.mode());
      }
      requestRender();
      return;
  }
}

void RendererRuntime::handleButtons(uint32_t nowMs) {
  buttons_.update();
  if (buttons_.wasShortPressed(Buttons::ButtonId::BTN1)) {
    handleButtonPress(Buttons::ButtonId::BTN1, nowMs);
  }
  if (buttons_.wasShortPressed(Buttons::ButtonId::BTN2)) {
    handleButtonPress(Buttons::ButtonId::BTN2, nowMs);
  }
  if (buttons_.wasShortPressed(Buttons::ButtonId::BTN3)) {
    handleButtonPress(Buttons::ButtonId::BTN3, nowMs);
  }
}

void RendererRuntime::renderCurrent(const DateTime& now, uint32_t nowMs) {
  if (hostFrameActive_) {
    renderer_.render(hostFrame_);
  } else {
    SegmentFrame frame;
    app_.buildFrame(frame, settings_.settings(), now, nowMs);
    renderer_.render(frame);
  }
  renderRequested_ = false;
}

void RendererRuntime::update(uint32_t nowMs, const DateTime& now) {
  handleButtons(nowMs);

  if (controlMode_ == RendererControlMode::HOST &&
      (nowMs - lastHostActivityMs_) >= kHostTimeoutMs) {
    controlMode_ = RendererControlMode::LOCAL;
    hostFrameActive_ = false;
    hostTimeoutLatched_ = true;
    requestRender();
  }

  if (!hostFrameActive_ && app_.update(nowMs)) {
    requestRender();
  }

  if (!hostFrameActive_ && app_.consumeTimerDoneEvent()) {
    timerDoneLatched_ = true;
    requestRender();
  }

  if (renderRequested_) {
    renderCurrent(now, nowMs);
  }
}
