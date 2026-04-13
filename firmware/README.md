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
