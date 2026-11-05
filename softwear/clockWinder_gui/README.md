# clockWinder

The desktop app for setting up, testing, mapping, and controlling the NeoPixel clock over USB serial. It's built with PySide6 and pyserial.

## Run

```bash
pip install -r requirements.txt
python main.py
```

## Layout

- `main.py`: entry point
- `clockWinder/app.py`: Qt application setup
- `clockWinder/windows/main_window.py`: clock control (modes, colors, brightness, RTC, timer, stopwatch)
- `clockWinder/windows/calibration_window.py`: LED mapping workflow and JSON / header export
- `clockWinder/protocol.py`, `clockWinder/serial_worker.py`: serial protocol and background serial I/O
- `clockWinder/model.py`, `clockWinder/exports.py`: mapping model plus `ledmap.json` / `Generated_LedMap.h` import and export
- `clockWinder/widgets/`: reusable widgets (color button, log, mapping table, status)
- `clockWinder/ledmaps/`: saved LED maps. This is the default export folder.

## Mapping workflow

1. Flash `firmware/arduino/clock_calibration_firmware`.
2. Open the calibration window and connect.
3. For each lit physical LED, assign the matching segment.
4. Validate, then **Save** to EEPROM on the board.
5. Optionally export `Generated_LedMap.h` and copy it to `firmware/arduino/clock_firmware/src/display/` so it becomes the built-in default.
