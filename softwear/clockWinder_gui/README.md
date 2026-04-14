# ClockWinder Desktop GUI

This is the PySide6 desktop control app for the clock project.

Entry point:

- `main.py`

Main package:

- `clock_gui/`

## What it does

The GUI is the desktop-side console for the Leonardo-era clock workflow. It supports:

- serial connection with `pyserial`
- `HELLO`, `INFO?`, and `STATUS?`
- calibration mode workflow
- mapping table editing
- JSON and header export
- runtime mode control
- brightness and colon brightness control
- color control for clock, timer, and stopwatch targets
- RTC read/set
- timer and stopwatch controls

## Package layout

- `clock_gui/app.py` - Qt application setup and styling
- `clock_gui/protocol.py` - line-based serial command helpers and response parsing
- `clock_gui/model.py` - local mapping and device state models
- `clock_gui/serial_worker.py` - worker-thread serial I/O
- `clock_gui/exports.py` - JSON/header export helpers
- `clock_gui/constants.py` - shared logical IDs, modes, presets, and UI constants
- `clock_gui/widgets/` - reusable widgets
- `clock_gui/windows/main_window.py` - main application window

## Run

Install:

```bash
pip install PySide6 pyserial
```

Launch:

```bash
python main.py
```

## Position In The Project

This app is still very relevant for the Leonardo firmware path.

For the long-term ESP32 path, the plan is different:

- the clock serves its own browser UI
- the ESP32 becomes the main configuration surface
- the desktop GUI becomes optional rather than mandatory

That means this app should stay clean and usable, but it does not need to become the long-term primary UX if the ESP32 web path takes over.
