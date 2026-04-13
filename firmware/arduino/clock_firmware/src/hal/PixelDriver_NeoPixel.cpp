#include "PixelDriver_NeoPixel.h"

PixelDriver_NeoPixel::PixelDriver_NeoPixel(uint8_t pin, uint16_t count)
  : mPixels(count, pin, NEO_GRB + NEO_KHZ800) {}

void PixelDriver_NeoPixel::begin() { mPixels.begin(); }
void PixelDriver_NeoPixel::setBrightness(uint8_t brightness) { mPixels.setBrightness(brightness); }
void PixelDriver_NeoPixel::clear() { mPixels.clear(); }
void PixelDriver_NeoPixel::show() { mPixels.show(); }
void PixelDriver_NeoPixel::setPixelColor(uint16_t index, uint32_t color) { mPixels.setPixelColor(index, color); }
uint32_t PixelDriver_NeoPixel::color(uint8_t r, uint8_t g, uint8_t b) const { return mPixels.Color(r, g, b); }
uint16_t PixelDriver_NeoPixel::count() const { return mPixels.numPixels(); }
