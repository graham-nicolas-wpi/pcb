#include "Face_Timer.h"

void Face_Timer::render(const FaceRenderState& state) {
  RgbColor digitColors[4];
  for (uint8_t i = 0; i < 4; ++i) {
    digitColors[i] = scaleColor(state.digitColors[i], state.settings.brightness);
  }

  state.frame.clear();
  state.frame.renderDigits(state.digits, digitColors, state.blankLeadingDigit);
  state.frame.setColon(state.colonTopOn, state.colonBottomOn,
                       scaleColor(state.colonColor, state.settings.colonBrightness));
  state.frame.setDecimal(state.decimalOn,
                         scaleColor(state.decimalColor, state.settings.brightness));
}
