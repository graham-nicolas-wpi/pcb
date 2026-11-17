# Firmware Overview

This repo now has one permanent two-MCU product architecture:

- Leonardo on the custom PCB as the renderer/runtime coprocessor
- ESP32 as the external host platform over header-accessible UART

## Leonardo Firmware

Folders:

- `arduino/clock_firmware/` - normal renderer runtime using `ClockRender/1`
- `arduino/clock_calibration_firmware/` - calibration and map-writing firmware using `ClockCal/1`

Leonardo responsibilities:

- render the 31 logical targets on the onboard WS2812 chain
- keep physical mapping and calibration board-native
- read the board buttons on `D8`, `D9`, `D10`
- read the DS3231 over I2C
- support stable runtime commands and `FRAME24` rendering
- provide a useful local fallback mode when the ESP32 host is absent

Why runtime and calibration remain split:

- ATmega32U4 flash is tight
- calibration tools are still important, but they do not belong in every everyday runtime build
- keeping calibration separate makes the normal runtime leaner and more durable

## ESP32 Firmware

Folder:

- `platformio/esp32_clock/`

ESP32 responsibilities:

- own saved presets, faces, widgets, and schedules
- own LittleFS config and browser-based UX
- own NTP, network policy, and future integrations
- own command generation toward the Leonardo renderer
- compile high-level host features into stable renderer commands or `FRAME24`

The active ESP32 path no longer assumes direct control of the NeoPixel chain, button wiring, or RTC wiring on the custom board.

## Wiring Contract

Preferred host link:

- ESP32 `GPIO17 TX` -> Leonardo `D0 / RX`
- Leonardo `D1 / TX` -> ESP32 `GPIO16 RX` through level protection
- common `GND`

This architecture avoids PCB redesign and avoids soldering to hidden custom-board traces.

## Desktop App Relationship

The desktop app in `../softwear/clockWinder_gui/` still matters for:

- direct Leonardo runtime control
- calibration using the separate calibration sketch
- bench testing and development

The long-term primary UX for product features is the ESP32 web/API stack.

## What To Use

### Board bring-up

Use Leonardo runtime plus the calibration sketch.

### Product feature work

Use the ESP32 PlatformIO project and treat the Leonardo as a renderer coprocessor.

## Reference

- canonical architecture and protocol: [../docs/leonardo-renderer-esp32-host.md](../docs/leonardo-renderer-esp32-host.md)
