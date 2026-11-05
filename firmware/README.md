# Firmware

Arduino firmware for the NeoPixel smart clock. Target: **Arduino Leonardo / ATmega32U4**.

## Sketches

| Sketch | Purpose |
| --- | --- |
| `arduino/clock_firmware/clock_firmware.ino` | The clock: modes, buttons, RTC, settings, and the serial control protocol |
| `arduino/clock_calibration_firmware/clock_calibration_firmware.ino` | LED mapping: steps through each physical LED and assigns it a logical segment |

The calibration sketch doesn't keep its own copy of the shared code. Its `src/**/*.cpp` files are one-line wrappers that `#include` the sources from `clock_firmware/src/`. Keep the two sketch folders next to each other.

Libraries (Library Manager): **Adafruit NeoPixel**, plus **RTClib** for `clock_firmware`.

Example build with arduino-cli:

```bash
arduino-cli compile --fqbn arduino:avr:leonardo firmware/arduino/clock_firmware
arduino-cli compile --fqbn arduino:avr:leonardo firmware/arduino/clock_calibration_firmware
```

## Source layout (`clock_firmware/src/`)

- `hal/`: buttons (debounced, active low), NeoPixel driver, DS3231 wrapper
- `display/`: logical segment IDs, LED map (EEPROM-backed), logical display rendering, `Generated_LedMap.h` default map
- `calibration/`: `CalibrationController` (mapping) and `ClockProtocol` (clock serial commands)
- `settings/`: EEPROM settings store and color/preset profiles
- `ui/`: mode state machine
- `faces/`: clock and timer face rendering
- `util/`: color and checksum helpers

## Pins

| Function | Pin |
| --- | --- |
| NeoPixel data | D5 |
| Buttons 1/2/3 | D8 / D9 / D10 (active low) |
| DS3231 SDA / SCL | D2 / D3 |

## LED map

The map goes from physical LED index (position on the data chain) to logical segment (`LogicalIds.h`: `D1_A`...`D4_G`, `COLON_TOP`, `COLON_BOTTOM`, `DECIMAL`). At boot, both sketches load the map from EEPROM if it's valid. Otherwise they use `kPhysToLogical` from `Generated_LedMap.h`. The inverse table is always rebuilt at runtime.

## Serial protocol (115200 baud, newline-terminated)

### Clock firmware

- `HELLO`, `INFO?`, `STATUS?`
- `MODE CLOCK | CLOCK_SECONDS | TIMER | STOPWATCH | COLOR_DEMO`
- `HOURMODE 12H | 24H`
- `SECONDSMODE BLINK | ON | OFF`
- `TIMERFORMAT AUTO | HHMM | MMSS | SSCS`
- `BRIGHTNESS <0-255>`, `COLONBRIGHTNESS <0-255>`
- `COLOR <target> <r> <g> <b>`. Targets: `CLOCK HOURS MINUTES DIGIT1..4 ACCENT SECONDS TIMER STOPWATCH TIMER_HOURS TIMER_MINUTES TIMER_SECONDS STOPWATCH_HOURS STOPWATCH_MINUTES STOPWATCH_SECONDS`
- `PRESET RAINBOW | PARTY`
- `RTC READ`, `RTC SET <yyyy> <mm> <dd> <hh> <mm> <ss>`
- `TIMER SET <seconds>`, `TIMER START | STOP | RESET`
- `STOPWATCH START | STOP | RESET`

### Calibration firmware

- `HELLO`, `INFO?`, `STATUS?`, `MODE CAL | RUN`
- `CUR?`, `SET <phys>`, `NEXT`, `PREV`
- `ASSIGN <logical>`, `UNASSIGN_CUR`
- `MAP?`, `VALIDATE?`, `SAVE`, `LOAD`
- `TEST DIGITS | SEGMENTS | ALL`
