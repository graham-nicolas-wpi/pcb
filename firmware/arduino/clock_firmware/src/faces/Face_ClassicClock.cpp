#include "Face_ClassicClock.h"

#include "../util/Color.h"

void Face_ClassicClock::render(const FaceRenderState& state) {
  RgbColor digitColors[4];
  RgbColor colonColor = state.colonColor;

  for (uint8_t i = 0; i < 4; ++i) {
    digitColors[i] = scaleColor(state.digitColors[i], state.settings.brightness);
  }

  if (state.rainbowDigits) {
    const uint8_t base = static_cast<uint8_t>(state.animationStep * 5U);
    digitColors[0] = scaleColor(colorFromHue(base + 0U), state.settings.brightness);
    digitColors[1] = scaleColor(colorFromHue(base + 32U), state.settings.brightness);
    digitColors[2] = scaleColor(colorFromHue(base + 96U), state.settings.brightness);
    digitColors[3] = scaleColor(colorFromHue(base + 160U), state.settings.brightness);
    colonColor = colorFromHue(base + 224U);
  }

  state.frame.clear();
  state.frame.renderDigits(state.digits, digitColors, state.blankLeadingDigit);
  state.frame.setColon(state.colonTopOn, state.colonBottomOn,
                       scaleColor(colonColor, state.settings.colonBrightness));
  state.frame.setDecimal(state.decimalOn,
                         scaleColor(state.decimalColor, state.settings.brightness));
}
