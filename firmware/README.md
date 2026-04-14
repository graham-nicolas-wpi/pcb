# ClockCal starter project

This is a generated starter project for your NeoPixel four-digit clock.

## Hardware assumptions
- Arduino Leonardo / ATmega32U4
- 31 WS2812 LEDs on `D5`
- 3 active-low buttons on `D8`, `D9`, `D10`
- Future DS3231 RTC on I2C

## Folder layout
- `firmware/ClockCal_Firmware/` — Arduino firmware starter
- `gui/clockcal_tool.py` — desktop calibration GUI starter
- `gui/requirements.txt` — Python dependencies

## First steps
1. Install Arduino library `Adafruit NeoPixel`.
2. Open `ClockCal_Firmware.ino` in Arduino IDE.
3. Upload to your Leonardo.
4. In Serial Monitor, try:
   - `HELLO ClockCal/1`
   - `INFO?`
   - `NEXT`
   - `ASSIGN 0`
   - `MAP?`
   - `TEST SEGMENTS`
5. For the GUI:
   - `pip install -r gui/requirements.txt`
   - `python gui/clockcal_tool.py`

## Notes
- `Generated_LedMap.h` starts as an identity-map placeholder.
- After calibration, export a real header from the GUI and replace it.

# Firmware

This folder contains the Arduino-side firmware for the **Modular NeoPixel Smart Clock**.

The firmware is responsible for driving the 31-pixel 4-digit display, handling button input, reading and writing RTC time, storing settings, and responding to commands from the desktop GUI.

## Current hardware target

- **Arduino Leonardo / ATmega32U4**
- **31 WS2812 / NeoPixel LEDs** on `D5`
- **3 active-low buttons** on `D8`, `D9`, and `D10`
- **DS3231 RTC** on I2C

## Purpose of this firmware

The firmware is meant to grow beyond a one-off class sketch into a cleaner modular clock codebase.

Current goals:

- drive the custom 4-digit NeoPixel clock display
- support saved logical LED mapping
- support RTC-backed timekeeping
- provide timer and stopwatch modes
- support configurable colors and brightness
- support serial control from the desktop GUI
- keep the architecture clean enough to expand later

## Expected structure

This firmware area is being cleaned up into a more modular layout.

Planned structure:

- `arduino/` — Arduino IDE project files
- `src/hal/` — hardware abstraction layers like buttons, pixels, and RTC
- `src/display/` — logical IDs, LED mapping, and display rendering
- `src/calibration/` — calibration protocol and mapping control
- `src/settings/` — EEPROM-backed settings and profiles
- `src/ui/` — mode handling and state machine logic
- `src/faces/` — clock face and mode-specific rendering

## Current important files

Examples of the files this firmware area is centered around:

- `ClockCal_Firmware.ino` or renamed firmware entry file
- `Generated_LedMap.h`
- button handling files
- logical display / mapping files
- calibration controller files
- RTC support files

## Hardware assumptions

### Display and buttons

- NeoPixel data line -> `D5`
- Button 1 -> `D8`
- Button 2 -> `D9`
- Button 3 -> `D10`
- buttons are active-low and connect to GND when pressed

### DS3231 RTC

- `VCC` -> `5V`
- `GND` -> `GND`
- `SDA` -> `D2 / SDA`
- `SCL` -> `D3 / SCL`

## Basic workflow

1. Open the firmware project in Arduino IDE.
2. Select **Arduino Leonardo**.
3. Install the required libraries.
4. Upload the firmware.
5. Connect with the desktop GUI or Serial Monitor.
6. Verify the mapping, RTC, and display modes.

## Useful serial commands

Examples:

- `HELLO ClockCal/1`
- `INFO?`
- `STATUS?`
- `NEXT`
- `PREV`
- `ASSIGN 0`
- `MAP?`
- `TEST SEGMENTS`
- `TEST DIGITS`
- `SAVE`

## Notes

- `Generated_LedMap.h` should eventually contain the real calibrated logical-to-physical mapping.
- EEPROM-backed mapping and settings are important parts of this firmware design.
- This folder README should describe the firmware specifically, while the root README explains the full project.