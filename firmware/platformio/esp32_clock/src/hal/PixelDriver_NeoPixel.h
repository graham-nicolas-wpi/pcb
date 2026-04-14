#pragma once

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

class PixelDriver_NeoPixel {
 public:
  PixelDriver_NeoPixel(uint8_t pin, uint16_t count);

  void begin();
  void setBrightness(uint8_t brightness);
  void clear();
  void show();
  void setPixelColor(uint16_t index, uint32_t color);
  uint32_t color(uint8_t r, uint8_t g, uint8_t b) const;
  uint16_t count() const;

 private:
  Adafruit_NeoPixel pixels_;
};
