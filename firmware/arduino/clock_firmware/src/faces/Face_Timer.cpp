#include "Face_Timer.h"

void Face_Timer::render(const FaceRenderState& state) {
  uint32_t digitColors[4];
  for (uint8_t i = 0; i < 4; ++i) {
    digitColors[i] = toColorScaled(state.pixels, state.digitColors[i], state.settings.brightness);
  }

  state.display.clear();
  state.display.renderDigits(state.digits, digitColors, state.blankLeadingDigit);
  state.display.setColon(
      state.colonTopOn, state.colonBottomOn,
      toColorScaled(state.pixels, state.colonColor, state.settings.colonBrightness));
  state.display.setDecimal(
      state.decimalOn,
      toColorScaled(state.pixels, state.decimalColor, state.settings.brightness));
  state.display.show();
}
