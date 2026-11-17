# Modular NeoPixel Smart Clock

This repository contains the full clock project for a custom four-digit NeoPixel display board and its long-term two-MCU software stack.

## Permanent Hardware Direction

The custom PCB stays physically unchanged.

- Arduino Leonardo remains on the board as the permanent renderer and board-native hardware controller
- ESP32-WROOM-32E is the external high-level host platform
- ESP32 talks to Leonardo over exposed header pins only
- ESP32 does not directly drive `LED_DIN`
- no PCB redesign and no soldering to hidden custom-board traces are part of the main plan

Preferred UART link:

- ESP32 `GPIO17 TX` -> Leonardo `D0 / RX`
- Leonardo `D1 / TX` -> ESP32 `GPIO16 RX` through a divider or level shifter
- common `GND`

## Repo Layout

- `hardware/pcb/` - KiCad sources, fabrication outputs, schematic exports
- `firmware/arduino/clock_firmware/` - Leonardo renderer runtime using the `ClockRender/1` protocol
- `firmware/arduino/clock_calibration_firmware/` - Leonardo calibration and map-writing firmware using `ClockCal/1`
- `firmware/platformio/esp32_clock/` - ESP32 host platform with web UI, storage, time/network logic, and renderer link control
- `softwear/clockWinder_gui/` - desktop serial tools for Leonardo runtime and calibration workflows
- `docs/leonardo-renderer-esp32-host.md` - canonical coprocessor architecture and serial/render contract

## MCU Responsibilities

### Leonardo

The Leonardo is the stable board appliance:

- renders digits, colon, and decimal to the onboard NeoPixels
- keeps LED mapping and physical calibration authoritative on-board
- reads board buttons and RTC
- exposes a compact serial contract
- supports local fallback clock/timer/stopwatch behavior when no host is attached

### ESP32

The ESP32 is the product brain:

- owns presets, faces, widgets, schedules, and runtime config
- owns NTP, Wi-Fi, storage, browser UI, and future integrations
- compiles rich features into stable renderer commands or `FRAME24` payloads
- treats the Leonardo as a renderer coprocessor instead of replacing it

## Recommended Reading

- Firmware overview: [firmware/README.md](firmware/README.md)
- Leonardo/ESP32 contract: [docs/leonardo-renderer-esp32-host.md](docs/leonardo-renderer-esp32-host.md)
- ESP32 host overview: [firmware/platformio/esp32_clock/README.md](firmware/platformio/esp32_clock/README.md)
- ESP32 host architecture: [firmware/platformio/esp32_clock/ARCHITECTURE.md](firmware/platformio/esp32_clock/ARCHITECTURE.md)
- Desktop GUI notes: [softwear/clockWinder_gui/README.md](softwear/clockWinder_gui/README.md)

## Recommended Workflow

### Bring up the board renderer

1. Flash `firmware/arduino/clock_firmware/` to the Leonardo for normal operation
2. Flash `firmware/arduino/clock_calibration_firmware/` only when you need to remap LEDs
3. Use the desktop GUI for direct Leonardo control or calibration

### Build the long-term product platform

1. Work in `firmware/platformio/esp32_clock/`
2. Connect the ESP32 to the Leonardo over the header UART link
3. Add new features on the ESP32 and emit `ClockRender/1` commands or `FRAME24` frames

## Project Status

- The Leonardo runtime is now being stabilized as a durable renderer/runtime coprocessor.
- The ESP32 project is the long-term expansion surface for nearly all new features.
- The calibration workflow remains Leonardo-owned so physical board mapping stays tied to the actual renderer MCU.
