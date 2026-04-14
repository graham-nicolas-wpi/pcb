# ESP32 Clock Overhaul

This PlatformIO project is the long-term ESP32-first architecture for the custom four-digit NeoPixel clock.

It keeps the original clock concepts:

- 31 logical LED targets for the four-digit display
- saved LED mapping / calibration
- clock, timer, stopwatch, and color demo modes
- independent colon brightness
- DS3231 RTC support
- serial command compatibility for the current desktop tooling

It adds a new ESP32-native direction:

- browser-based configuration UI served directly from the clock
- saved presets, faces, and widgets
- richer effect system
- Wi-Fi access point plus optional station credentials
- JSON-backed storage in LittleFS
- modular firmware structure for long-term expansion

## Recommended ESP32 wiring

For `ESP32-WROOM-32E`, do not plan around the old Leonardo button numbering. This project defaults to:

- NeoPixel data: `GPIO18`
- Button 1: `GPIO25`
- Button 2: `GPIO26`
- Button 3: `GPIO27`
- DS3231 SDA: `GPIO21`
- DS3231 SCL: `GPIO22`

These defaults are editable in saved config, but hardware pin changes require reboot.

## Project layout

- `src/main.cpp` - startup and loop
- `src/model/ClockConfig.h` - persistent data model
- `src/storage/SettingsStore.*` - LittleFS JSON persistence and exports
- `src/runtime/ClockRuntime.*` - rendering, timer, stopwatch, buttons, calibration
- `src/protocol/SerialProtocol.*` - serial command compatibility
- `src/web/WebUiServer.*` - HTTP API and static file hosting
- `src/display/*` - mapping and logical display
- `src/hal/*` - buttons and NeoPixel driver
- `data/*` - browser UI assets uploaded to LittleFS

## Upload workflow

1. Install PlatformIO.
2. Build and upload firmware:
   - `pio run -t upload`
3. Upload web assets:
   - `pio run -t uploadfs`
4. Open serial monitor:
   - `pio device monitor`

## First boot behavior

The device starts a Wi-Fi access point using the saved AP credentials. Default values are:

- SSID: `NeoPixelClock`
- Password: `clockclock`

Open:

- `http://192.168.4.1/`

The UI can then edit presets, widgets, faces, mapping, network settings, and runtime controls.

## Architecture notes

This first ESP32 pass uses composable faces and widgets rather than user-scripted drawing code.

- Faces choose the base mode, preset, effect, and widget composition.
- Widgets are reusable render behaviors such as pulsing colon, rainbow digits, or timer warnings.
- Presets hold named color palettes and a default effect.

That gives you a practical customization model now, while leaving room for a future scripted face engine if you want one later.
