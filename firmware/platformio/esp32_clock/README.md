# ESP32 Clock Overhaul

This PlatformIO project is the long-term ESP32-first architecture for the custom four-digit NeoPixel clock.

It keeps the original clock concepts:

- 31 logical LED targets for the four-digit display
- saved LED mapping / calibration
- clock, timer, stopwatch, and color demo modes
- independent colon brightness
- DS3231 RTC support
- serial command compatibility for the current desktop tooling

It adds the new ESP32-native direction:

- browser-based configuration UI served directly from the clock
- saved presets, faces, and widgets
- richer effect system
- Wi-Fi access point plus optional station credentials
- JSON-backed storage in LittleFS
- modular firmware structure for long-term expansion

## Current Goal

This project is the foundation for the long-term branch, not just a quick experimental sketch. The point is to create a structure that can reasonably grow into:

- a customizable network-connected clock
- a self-hosted configuration surface
- reusable saved themes / presets
- richer display behaviors than the Leonardo can comfortably hold

At the same time, it is still early enough that it should be treated as an active architecture build-out rather than a fully settled final product.

## Recommended ESP32 Wiring

For `ESP32-WROOM-32E`, do not plan around the old Leonardo button numbering. This project defaults to:

- NeoPixel data: `GPIO18`
- Button 1: `GPIO25`
- Button 2: `GPIO26`
- Button 3: `GPIO27`
- DS3231 SDA: `GPIO21`
- DS3231 SCL: `GPIO22`

These defaults are editable in saved config, but hardware pin changes require reboot. They are chosen to avoid the worst ESP32 pin pitfalls and to keep the project away from flash and strap-pin trouble.

## Project Layout

- `src/main.cpp` - startup and loop
- `src/model/ClockConfig.h` - persistent data model
- `src/storage/SettingsStore.*` - LittleFS JSON persistence and exports
- `src/runtime/ClockRuntime.*` - rendering, timer, stopwatch, buttons, calibration
- `src/protocol/SerialProtocol.*` - serial command compatibility
- `src/web/WebUiServer.*` - HTTP API and static file hosting
- `src/display/*` - mapping and logical display
- `src/hal/*` - buttons and NeoPixel driver
- `data/*` - browser UI assets uploaded to LittleFS

For a more detailed explanation of how the layers fit together, see:

- [ARCHITECTURE.md](ARCHITECTURE.md)

## Current Feature Set

The current first-pass ESP32 project includes:

- runtime modes for clock, clock-seconds, timer, stopwatch, and color demo
- saved presets with palette-based color configuration
- saved widgets and saved faces
- timer / stopwatch / RTC controls in the web UI
- mapping, calibration cursor, assign/unassign, and exports
- config save/load to LittleFS
- serial command compatibility for many of the existing Leonardo-era commands

## Upload Workflow

1. Install PlatformIO.
2. Build and upload firmware:
   - `pio run -t upload`
3. Upload web assets:
   - `pio run -t uploadfs`
4. Open serial monitor:
   - `pio device monitor`

## First Boot Behavior

The device starts a Wi-Fi access point using the saved AP credentials. Default values are:

- SSID: `NeoPixelClock`
- Password: `clockclock`

Open:

- `http://192.168.4.1/`

The UI can then edit presets, widgets, faces, mapping, network settings, and runtime controls.

## Browser UI Scope

The browser UI is meant to be the main long-term control surface. It already covers:

- live runtime control
- timer / stopwatch / RTC control
- presets
- widgets
- faces
- mapping and calibration
- config export
- network settings

That makes it different from the older desktop GUI path, which is still useful but no longer has to be the primary UX for the future platform.

## Architecture Notes

This first ESP32 pass uses composable faces and widgets rather than user-scripted drawing code.

- Faces choose the base mode, preset, effect, and widget composition.
- Widgets are reusable render behaviors such as pulsing colon, rainbow digits, or timer warnings.
- Presets hold named color palettes and a default effect.

That gives you a practical customization model now, while leaving room for a future scripted face engine if you want one later.

## Improvements Still Worth Making

- run real local PlatformIO compile/upload verification and tighten any library/API mismatches
- add schema/version migration notes for saved config
- harden the HTTP API contracts if outside tools will depend on them
- add screenshots of the web UI and wiring diagrams for the recommended ESP32 build
- decide how far you want to go with user-defined faces before introducing a scripting layer
