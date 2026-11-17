#pragma once

#include <Arduino.h>
#include <RTClib.h>

#include "../hal/Buttons.h"
#include "../render/SegmentFrame.h"

class LogicalDisplay;
class PixelDriver_NeoPixel;
class SettingsStore;
class AppStateMachine;
class SegmentFrameRenderer;

enum class RendererControlMode : uint8_t {
  LOCAL = 0,
  HOST,
};

struct ButtonEvent {
  Buttons::ButtonId buttonId;
};

class RendererRuntime {
 public:
  RendererRuntime(Buttons& buttons, SettingsStore& settings, AppStateMachine& app,
                  SegmentFrameRenderer& renderer);

  void begin(uint32_t nowMs);
  void update(uint32_t nowMs, const DateTime& now);
  void requestRender();
  void noteHostActivity(uint32_t nowMs);
  void setControlMode(RendererControlMode mode, uint32_t nowMs);
  RendererControlMode controlMode() const;
  const char* controlModeName() const;

  void useLocalRenderer(uint32_t nowMs);
  void setHostFrame(const SegmentFrame& frame, uint32_t nowMs);
  bool hostFrameActive() const;

  void setButtonReporting(bool enabled);
  bool buttonReportingEnabled() const;

  bool consumeButtonEvent(ButtonEvent& event);
  bool consumeTimerDoneEvent();
  bool consumeHostTimeoutEvent();

  static constexpr uint32_t kHostTimeoutMs = 5000UL;

 private:
  void renderCurrent(const DateTime& now, uint32_t nowMs);
  void handleButtons(uint32_t nowMs);
  void handleButtonPress(Buttons::ButtonId buttonId, uint32_t nowMs);
  void queueButtonEvent(Buttons::ButtonId buttonId);

  Buttons& buttons_;
  SettingsStore& settings_;
  AppStateMachine& app_;
  SegmentFrameRenderer& renderer_;
  RendererControlMode controlMode_ = RendererControlMode::LOCAL;
  bool buttonReportingEnabled_ = false;
  bool hostFrameActive_ = false;
  bool renderRequested_ = true;
  uint32_t lastHostActivityMs_ = 0;
  uint8_t pendingButtonMask_ = 0;
  bool timerDoneLatched_ = false;
  bool hostTimeoutLatched_ = false;
  SegmentFrame hostFrame_;
};
