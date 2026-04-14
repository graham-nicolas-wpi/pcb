# Firmware Overview

This folder now contains two distinct firmware directions:

1. Leonardo-based Arduino IDE firmware for the current clock hardware
2. ESP32-based PlatformIO firmware for the long-term overhaul

They share the same clock concept, but they solve different problems.

## Arduino / Leonardo Firmware

Current folders:

- `arduino/clock_firmware/` - normal runtime firmware
- `arduino/clock_calibration_firmware/` - dedicated calibration / mapping firmware

### Leonardo hardware assumptions

- 31 WS2812 LEDs on `D5`
- active-low buttons on `D8`, `D9`, `D10`
- DS3231 RTC on I2C
- EEPROM used for saved mapping and settings

### Why it is split now

The Leonardo is tight on flash. Calibration features and test tooling are useful, but they are also expensive to keep linked into the everyday runtime build. Splitting calibration into its own sketch keeps the runtime leaner and makes future maintenance more manageable on ATmega32U4.

### Runtime firmware responsibilities

- render the 31-pixel logical display
- run clock / timer / stopwatch / color demo modes
- persist settings and mapping
- read the DS3231 if present
- expose the serial control surface used by the desktop GUI

### Calibration firmware responsibilities

- LED-by-LED mapping workflow
- assignment and validation
- map save/load
- segment and digit tests
- `ClockCal/1` style calibration handshake

## Desktop App Relationship

The current desktop app lives in:

- `../softwear/clockWinder_gui/`

It talks to the Leonardo firmware over serial and supports both runtime control and calibration workflows.

## ESP32 PlatformIO Firmware

Long-term folder:

- `platformio/esp32_clock/`

This is the expansion path for the project. It is designed around:

- ESP32-WROOM-32E
- LittleFS-backed saved config
- browser-based control UI served by the device
- presets, widgets, faces, and effects
- calibration and mapping in the web UI
- serial compatibility for the existing command vocabulary where it still helps

## Which One To Use

### Use Leonardo firmware when:

- you are working with the current existing clock board
- you want the proven hardware path
- you need Arduino IDE uploads and the current desktop GUI workflow

### Use the ESP32 project when:

- you are exploring the long-term redesign
- you want richer customization and web control
- you want room for much larger features than Leonardo can comfortably hold

## Improvement Notes

A few repo-wide improvements still worth doing:

- local PlatformIO compile/upload verification for the ESP32 project
- screenshots and usage docs for the web UI
- API/versioning notes for the ESP32 HTTP + serial interfaces
- eventual cleanup of duplicated clock concepts between the Leonardo and ESP32 code paths once the long-term direction settles
