#include "Face_ClassicClock.h"

#include "../util/Color.h"

void Face_ClassicClock::render(const FaceRenderState& state) {
  uint32_t digitColors[4];
  RgbColor colonColor = state.colonColor;

  for (uint8_t i = 0; i < 4; ++i) {
    digitColors[i] = toColorScaled(state.pixels, state.digitColors[i], state.settings.brightness);
  }

  if (state.rainbowDigits) {
    const uint8_t base = static_cast<uint8_t>(state.animationStep * 5U);
    digitColors[0] = toColorScaled(state.pixels, colorFromHue(base + 0U),
                                   state.settings.brightness);
    digitColors[1] = toColorScaled(state.pixels, colorFromHue(base + 32U),
                                   state.settings.brightness);
    digitColors[2] = toColorScaled(state.pixels, colorFromHue(base + 96U),
                                   state.settings.brightness);
    digitColors[3] = toColorScaled(state.pixels, colorFromHue(base + 160U),
                                   state.settings.brightness);
    colonColor = colorFromHue(base + 224U);
  }

  state.display.clear();
  state.display.renderDigits(state.digits, digitColors, state.blankLeadingDigit);
  state.display.setColon(
      state.colonTopOn, state.colonBottomOn,
      toColorScaled(state.pixels, colonColor, state.settings.colonBrightness));
  state.display.setDecimal(
      state.decimalOn,
      toColorScaled(state.pixels, state.decimalColor, state.settings.brightness));
  state.display.show();
}
