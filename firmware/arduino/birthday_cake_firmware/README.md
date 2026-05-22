# Birthday Cake Firmware

Arduino sketch for the birthday cake version of the board.

## Hardware assumptions

- Arduino Leonardo / ATmega32U4
- WS2812 / NeoPixel LEDs on `D5`
- 1 active-low button on ATmega32U4 pad 1 / `PE6` / Arduino Leonardo `D7`
- buzzer on `D13`

## LED order

The sketch currently assumes this physical NeoPixel order:

- pixels `0` through `8`: the 9 candle-tip LEDs, in the order they should light
- pixels `9` through `13`: the 5 top sparkle LEDs
- the driver is configured for 31 pixels so reused clock-board LED positions can still work

If the chain is wired differently, update these arrays near the top of
`birthday_cake_firmware.ino`:

```cpp
constexpr uint8_t kCandles[kCandleCount] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr uint8_t kSparkles[kSparkleCount] = {9, 10, 11, 12, 13};
```

## Buzzer mode

The buzzer behaves like a passive piezo: simply turning `D13` on and off makes
clicks, not a sustained tone. The sketch now creates a software square wave on
`D13` during each rhythm note. This board appears to be active-low, so `LOW` is
the driven side and `HIGH` is idle. By default, the buzzer follows a direct
Happy Birthday melody in a lower, cleaner range than the first fixed `2100 Hz`
test.

```cpp
constexpr bool kUseSoftwareSquareWave = true;
constexpr bool kUseMelodyPitch = true;
constexpr uint8_t kMelodyPitchScalePercent = 100;
```

## Behavior

Press the button to start or restart the show:

1. Candle 1 lights immediately.
2. The remaining 8 candle LEDs light one by one quickly.
3. The buzzer plays the "Happy Birthday" rhythm on `D13`.
4. While the song plays, the candle LEDs flicker gently and the 5 top LEDs twinkle in random colors.
5. When the song ends, the candle LEDs flicker strongly, dim and flare like wind is hitting them, then go out one by one in random order while the top LEDs fade down.

## Tuning

Useful constants near the top of `birthday_cake_firmware.ino`:

- `kCandleFlickerMs`: how quickly the candle flicker updates
- `kFinaleFlickerMs`: how long the big end flicker lasts
- `kConfettiFadeMs`: how long the top LEDs fade after the candles go out
- `kDurationScalePercent`: song rhythm length; `100` is original speed, higher is slower
- `kRestMs`: quiet gap between beeps; lower this if the song feels too choppy
- `kMelodyPitchScalePercent`: whole-song pitch; `100` is current melody pitch, try `125` or `150` only if it sounds too low/quiet
- `kUseMelodyPitch`: set to `false` to use one fixed pitch instead of different notes
- `kFixedBuzzFrequencyHz`: fixed pitch used when `kUseMelodyPitch` is `false`
