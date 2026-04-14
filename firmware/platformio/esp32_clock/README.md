# ESP32 Clock Host

This PlatformIO project is the authoritative high-level host for the clock system. It does not directly drive the custom PCB LED chain.

## Host Role

The ESP32 owns:

- presets, faces, widgets, and future feature composition
- LittleFS-backed configuration and saved state
- Wi-Fi AP/STA behavior
- browser UI and HTTP API
- NTP and other future time sources
- UART communication with the Leonardo renderer

The ESP32 does not own:

- board NeoPixel `DIN`
- board button wiring
- the authoritative LED calibration map
- the board-native RTC wiring path

Those remain Leonardo responsibilities on this board revision.

## Wiring

Preferred renderer link:

- ESP32 `GPIO17 TX` -> Leonardo `D0 / RX`
- Leonardo `D1 / TX` -> ESP32 `GPIO16 RX` through a divider or level shifter
- common `GND`

This is the intended no-PCB-redesign and no-hidden-trace-solder path.

## Project Layout

- `src/main.cpp` - host startup and main loop
- `src/model/ClockConfig.h` - persisted host config model
- `src/storage/SettingsStore.*` - LittleFS JSON persistence
- `src/render/SegmentFrame.*` - stable logical-target frame model
- `src/render/FrameCompiler.*` - compiles presets/faces/widgets into a renderer frame
- `src/host/LeonardoClient.*` - UART client for `ClockRender/1`
- `src/runtime/HostRuntime.*` - host-owned timer, stopwatch, time source, and renderer orchestration
- `src/web/WebUiServer.*` - HTTP API and static file serving
- `data/*` - browser UI assets uploaded to LittleFS

## Renderer Contract

The ESP32 speaks to the Leonardo runtime using `ClockRender/1`.

Important commands include:

- `HELLO ClockRender/1`
- `STATUS?`
- `CONTROL HOST`
- `BTNREPORT 1`
- `RTC READ`
- `RTC SET ...`
- `MAP?`
- `TEST DIGITS|SEGMENTS|ALL`
- `FRAME24 <186 hex chars>`

For the full contract, see [../../../docs/leonardo-renderer-esp32-host.md](../../../docs/leonardo-renderer-esp32-host.md).

## Why This Split Matters

Future product features should usually land in one of these ESP32 layers:

- config model
- web/API surface
- storage and scheduling
- frame compiler
- renderer client/orchestration

The Leonardo should usually stay unchanged unless the stable renderer contract truly needs a new primitive.

## Upload Workflow

1. Install PlatformIO.
2. Build and upload firmware:
   - `pio run -t upload`
3. Upload web assets:
   - `pio run -t uploadfs`
4. Open serial monitor if needed:
   - `pio device monitor`

## First Boot

Default AP:

- SSID: `NeoPixelClock`
- Password: `clockclock`

Open:

- `http://192.168.4.1/`

The web UI edits host config and renderer-link settings, then drives the Leonardo renderer over UART.
