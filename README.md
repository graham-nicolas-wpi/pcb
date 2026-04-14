# Modular NeoPixel Smart Clock

This repository contains the full project for a custom 4-digit NeoPixel clock:

- custom PCB hardware
- Leonardo-based firmware and calibration tools
- a modular PySide6 desktop control app
- an in-progress ESP32-first long-term architecture with on-device web UI

The project started as a PCB course build and is now being pushed toward a more durable platform that can support richer faces, presets, widgets, network features, and future integrations.

## Current Structure

### Hardware

- `hardware/pcb/` - KiCad sources, fabrication outputs, schematic exports

### Arduino / Leonardo path

- `firmware/arduino/clock_firmware/` - lean runtime firmware for the existing Leonardo clock
- `firmware/arduino/clock_calibration_firmware/` - separate calibration/mapping firmware

This path is the practical option for the current board as it exists today:

- 31 WS2812 LEDs on `D5`
- 3 active-low buttons on `D8`, `D9`, `D10`
- DS3231 RTC on I2C
- EEPROM-backed settings and LED mapping
- serial control from the desktop app

### Desktop GUI

- `softwear/clockWinder_gui/` - PySide6 + pySerial desktop app

The desktop app supports:

- serial connect / disconnect
- runtime control for mode, brightness, colors, RTC, timer, stopwatch
- calibration workflow and mapping table
- JSON/header mapping exports
- a modular package layout for future work

### ESP32 path

- `firmware/platformio/esp32_clock/` - long-term ESP32-WROOM-32E architecture

This is the forward-looking direction for the project:

- ESP32-driven runtime
- LittleFS-backed saved config
- Wi-Fi AP + optional STA mode
- on-device browser UI
- presets, widgets, faces, effects, and mapping storage
- serial compatibility layer for existing tooling concepts

## Recommended Reading

- Leonardo firmware overview: [firmware/README.md](firmware/README.md)
- ESP32 project overview: [firmware/platformio/esp32_clock/README.md](firmware/platformio/esp32_clock/README.md)
- ESP32 architecture notes: [firmware/platformio/esp32_clock/ARCHITECTURE.md](firmware/platformio/esp32_clock/ARCHITECTURE.md)
- Desktop GUI notes: [softwear/clockWinder_gui/README.md](softwear/clockWinder_gui/README.md)

## Where The Project Stands

### Leonardo path

The Leonardo path is the stable path for the existing hardware, but it is now flash-constrained. Splitting calibration into its own firmware was the right move and buys more room, but long-term feature growth on ATmega32U4 will stay tight.

### ESP32 path

The ESP32 path is the long-term expansion path. It is meant to become the main platform for:

- richer faces and effects
- saved presets and customization
- web-based control instead of desktop-only control
- future data sources and integrations

Right now it is a strong architectural foundation, but it should still be treated as an active overhaul rather than the fully battle-tested shipping path.

## Suggested Workflow

### If you want the existing clock working now

1. Use the Leonardo runtime firmware in `firmware/arduino/clock_firmware/`
2. Use `firmware/arduino/clock_calibration_firmware/` only when mapping/calibration is needed
3. Use the PySide6 desktop app in `softwear/clockWinder_gui/`

### If you want the future platform

1. Work in `firmware/platformio/esp32_clock/`
2. Build out the ESP32 hardware around safe GPIO choices
3. Treat the web UI, presets, widgets, and faces as the main control model going forward

## Immediate Improvement Ideas

- Add a real PlatformIO build/test pass for the ESP32 project in your local environment
- Tighten the ESP32 docs and API contracts as the architecture settles
- Decide whether the ESP32 will fully replace Leonardo or act as the system brain while Leonardo remains a display coprocessor for one transitional revision
- Add screenshots / demo photos for both the GUI and ESP32 web UI
- Clean up repository naming over time if you want to rename `softwear/` to `software/`, but only when you are ready to touch imports, paths, and references together

## Notes

- The Leonardo and ESP32 paths are intentionally both in the repo right now.
- The ESP32 project is not just a port; it is a broader platform redesign.
- The branch you are working on can carry docs and architecture that are more ESP32-oriented than `main`, which is expected.
