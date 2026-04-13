# Smart Clock Package

This package gives you:

- `smart_clock_backend.ino` — Arduino Leonardo firmware for the 31-pixel 4-digit NeoPixel clock
- `smart_clock_gui.py` — desktop GUI to control colors, brightness, RTC time, timer, stopwatch, and test patterns
- support for the **hiLetgo DS3231 AT24C32 RTC module** with CR2032 backup battery

## What the firmware does

- uses your existing 31-LED calibrated mapping from EEPROM if it is already saved
- supports these modes:
  - Clock
  - MM:SS seconds display
  - Timer
  - Stopwatch
  - Color demo
- supports color changes for:
  - main clock digits
  - accent / colon
  - seconds mode
  - timer mode
- saves settings to EEPROM
- uses 3 active-low buttons on D8, D9, D10
- reads and writes time from the DS3231 RTC

## Leonardo wiring

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

### Important note about the battery

The **CR2032 goes into the RTC module itself**, not into the Leonardo or the clock PCB.
That battery keeps time when main power is removed.

## Button behavior on-device

### Button 1 (D8)
- short press: next display mode
- long press: toggle 12/24 hour mode

### Button 2 (D9)
- short press: rotate clock/accent colors
- long press: change brightness

### Button 3 (D10)
- in timer mode:
  - short press: add 1 minute
  - long press: start/stop timer
- in stopwatch mode:
  - short press: start/stop stopwatch
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

## GUI features

- connect to serial port
- set display mode
- change brightness
- toggle 12/24h, leading zero, colon blink
- read RTC time
- set RTC time from the GUI
- set RTC time from PC time
- set timer length and start/stop it
- start/stop/reset stopwatch
- change all main colors
- save settings to EEPROM
- run digit and segment tests
- read back mapping/status

## Serial commands the GUI uses

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

## If the time is wrong after power loss

That usually means one of these:

- the CR2032 is not installed correctly
- the battery is dead
- the RTC module is not wired to SDA/SCL correctly
- the RTC lost power before it was set at least once

## If the GUI will not connect

- close Arduino Serial Monitor first
- close any other serial tools
- unplug/replug the Leonardo
- refresh ports in the GUI
- choose the `/dev/cu.usbmodem...` or COM port for the Leonardo

