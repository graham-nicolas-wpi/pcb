# ESP32 Architecture Notes

This project is not meant to be a straight Leonardo port. It is a platform reset around the ESP32-WROOM-32E.

## Core Idea

The ESP32 project is built around one persisted configuration model that owns:

- hardware pin choices
- Wi-Fi settings
- LED mapping
- presets
- widgets
- faces
- active runtime state

That model is loaded from LittleFS, edited through the browser UI, and consumed by the runtime and serial/API layers.

## Layers

### `src/model`

The data model lives in:

- `src/model/ClockConfig.h`

This is the contract for the rest of the project. If you add a major new feature, it usually belongs here first.

### `src/storage`

- `src/storage/SettingsStore.*`

This layer persists the model to LittleFS as JSON and also provides export helpers for config, mapping JSON, and LED map headers.

### `src/display`

- `src/display/LogicalIds.h`
- `src/display/LedMap.*`
- `src/display/LogicalDisplay.*`

This layer preserves the logical display abstraction from the Leonardo architecture so the rest of the runtime can keep thinking in terms of digits, colon, decimal, and logical segments rather than raw physical pixel indices.

### `src/hal`

- `src/hal/Buttons.*`
- `src/hal/PixelDriver_NeoPixel.*`

This is the hardware access layer.

### `src/runtime`

- `src/runtime/ClockRuntime.*`

This is the runtime heart of the project. It is responsible for:

- timer and stopwatch state
- RTC usage
- calibration cursor and mapping edits
- preset / face / widget application
- rendering decisions
- on-device button behavior

### `src/protocol`

- `src/protocol/SerialProtocol.*`

This keeps a serial command surface available for continuity with the Leonardo era and for debugging / tooling.

### `src/web`

- `src/web/WebUiServer.*`

This serves the browser UI and the HTTP API.

### `data`

- `data/index.html`
- `data/app.css`
- `data/app.js`

These are the static assets uploaded to LittleFS and served by the ESP32.

## Faces, Widgets, Presets

The first ESP32 version uses a composable approach.

### Presets

Presets are saved color palettes plus a default effect tendency.

### Widgets

Widgets are reusable display behaviors:

- pulse colon
- rainbow digits
- timer warning
- seconds spark
- accent sweep

### Faces

Faces define:

- base mode
- default preset
- effect
- whether colon / decimal / seconds are shown
- which widgets are attached

This is intentionally simpler than a full scripting engine, but much easier to persist, edit, and stabilize.

## Recommended Expansion Path

If you keep growing the ESP32 project, a good next sequence is:

1. get the PlatformIO build fully green locally
2. verify LittleFS upload and browser UI round-trip on hardware
3. stabilize the HTTP API and config schema
4. improve the face/widget editor UX
5. decide whether custom scripted faces are worth the added complexity

## Design Constraints To Keep In Mind

- avoid turning the config schema into a grab-bag of one-off flags
- keep rendering logic centralized in the runtime layer
- keep serial and web actions as thin wrappers over runtime operations
- keep mapping/logical display abstractions intact
- preserve migration paths from Leonardo concepts where it reduces friction

## Recommended Mental Model

Think of the ESP32 project as:

- a device runtime
- a saved configuration system
- a local web application

all living in the same firmware package.

That is the biggest conceptual shift from the Leonardo path, and the docs should keep reinforcing it.
