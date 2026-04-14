# Modular NeoPixel Smart Clock

A custom 4-digit NeoPixel clock built around an **Arduino Leonardo**, a **31-pixel mapped display**, a **DS3231 RTC**, and a desktop control app for calibration, testing, and day-to-day control.

This repository is the home for the full project:

- custom PCB hardware
- Arduino firmware for the clock
- desktop GUI tooling
- RTC integration
- saved LED mapping and settings
- future expansion for sensors, smarter modes, and enclosure / 3D printed parts

## Project overview

This clock started as a PCB course project and is being developed into a more polished modular smart clock platform.

Right now, the system supports:

- a 4-digit / 31-LED NeoPixel display
- EEPROM-backed LED mapping
- DS3231 real-time clock support
- timer and stopwatch modes
- configurable colors and brightness
- 3-button on-device control
- a desktop control app for setup, testing, and live control

## Main parts of the project

### Firmware

The firmware runs on an **Arduino Leonardo** and drives the 31-pixel display using the saved logical LED mapping.

Current firmware capabilities:

- uses the saved LED mapping from EEPROM if available
- supports multiple display modes:
  - Clock
  - MM:SS seconds display
  - Timer
  - Stopwatch
  - Color demo
- supports configurable colors for:
  - main clock digits
  - accent / colon
  - seconds mode
  - timer mode
- saves settings to EEPROM
- uses 3 active-low buttons on D8, D9, and D10
- reads and writes time from the DS3231 RTC

### Desktop GUI

The desktop GUI is the control app for the project. It handles serial communication with the clock and makes it easier to test features without reflashing firmware every time.

Current GUI capabilities:

- connect to the Leonardo over serial
- set display mode
- change brightness
- toggle 12/24h, leading zero, and colon blink
- read RTC time
- set RTC time manually or from the PC time
- set timer length and start / stop it
- start / stop / reset the stopwatch
- change display colors
- save settings to EEPROM
- run digit and segment tests
- read back mapping / status

## Current files

This package currently includes:

- `smart_clock_backend.ino` — Arduino Leonardo firmware for the 31-pixel 4-digit NeoPixel clock
- `smart_clock_gui.py` — desktop GUI for colors, brightness, RTC time, timer, stopwatch, and test patterns
- support for the **hiLetgo DS3231 AT24C32 RTC module** with CR2032 backup battery

## Hardware wiring

### Existing clock board

Keep your existing connections the same:

- NeoPixel data -> `D5`
- Button 1 -> `D8`
- Button 2 -> `D9`
- Button 3 -> `D10`
- buttons are active-low, so each button should connect the pin to GND when pressed

### DS3231 RTC module wiring

Wire the hiLetgo DS3231 module like this:

- `DS3231 VCC` -> `Leonardo 5V`
- `DS3231 GND` -> `Leonardo GND`
- `DS3231 SDA` -> `Leonardo D2 / SDA`
- `DS3231 SCL` -> `Leonardo D3 / SCL`

Optional:

- `DS3231 SQW` -> leave unconnected for now
- `32K` -> leave unconnected

### Important battery note

The **CR2032 goes into the RTC module itself**, not into the Leonardo or the clock PCB.
That battery keeps time when main power is removed.

## Button behavior on-device

### Button 1 (`D8`)
- short press: next display mode
- long press: toggle 12/24 hour mode

### Button 2 (`D9`)
- short press: rotate clock / accent colors
- long press: change brightness

### Button 3 (`D10`)
- in timer mode:
  - short press: add 1 minute
  - long press: start / stop timer
- in stopwatch mode:
  - short press: start / stop stopwatch
  - long press: reset stopwatch
- in other modes:
  - short press: toggle colon blink

## Arduino libraries needed

Install these from Library Manager:

- Adafruit NeoPixel
- RTClib

## Uploading the firmware

1. Open `smart_clock_backend.ino` in Arduino IDE.
2. Select **Arduino Leonardo**.
3. Select the correct port.
4. Install the libraries if prompted.
5. Upload.

If your clock was already calibrated and the mapping was saved to EEPROM, the new firmware will try to use that saved mapping automatically.

## Running the GUI

### Python packages

Install:

```bash
pip install PySide6 pyserial
```

### Launch

```bash
python smart_clock_gui.py
```

## Serial commands used by the GUI

Examples:

```text
HELLO SmartClock/1
STATUS?
MODE 0
BRIGHTNESS 64
COLOR CLOCK FF7A18
COLOR ACCENT 2080FF
TIME 2026-04-12 14:30:00
TIME?
TIMER SET 300
TIMER START
STOPWATCH START
TEST DIGITS
TEST SEGMENTS
SAVE SETTINGS
```

## Good first test sequence

1. Wire the DS3231.
2. Insert the CR2032 into the DS3231 module.
3. Upload the firmware.
4. Open the GUI.
5. Connect to the Leonardo.
6. Click **Read RTC**.
7. Click **Set RTC From PC Time**.
8. Click **Set Mode -> Clock**.
9. Change the colors and brightness.
10. Click **Save Settings to EEPROM**.
11. Run **Digit Test** and **Segment Test** to confirm the saved calibration still matches.

## Troubleshooting

### If the time is wrong after power loss

That usually means one of these:

- the CR2032 is not installed correctly
- the battery is dead
- the RTC module is not wired to SDA / SCL correctly
- the RTC lost power before it was set at least once

### If the GUI will not connect

- close Arduino Serial Monitor first
- close any other serial tools
- unplug / replug the Leonardo
- refresh ports in the GUI
- choose the `/dev/cu.usbmodem...` or COM port for the Leonardo

## Next steps for the repo

As the project gets cleaned up, this repository will expand to better separate:

- hardware files and fabrication outputs
- firmware source modules
- GUI source code
- documentation and screenshots
- future enclosure / 3D printed parts

The long-term goal is to turn this from a class PCB submission into a polished engineering project with clean firmware architecture, cleaner desktop tooling, and a better GitHub portfolio presentation.
