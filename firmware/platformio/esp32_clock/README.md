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

Works on any 38-pin ESP32 DevKit (tested targets: ESP32-WROOM-32E and ESP32-WROVER-B).
GPIO16/17 are avoided because WROVER modules use them for PSRAM.

| ESP32 pin | Goes to | Notes |
| --- | --- | --- |
| GPIO26 (TX) | clock J2 pin 5 (D0 / RX) | direct wire |
| GPIO27 (RX) | clock J2 pin 4 (D1 / TX) | through a divider: 1 kOhm from J2 pin 4 to GPIO27, 2 kOhm from GPIO27 to GND |
| GND | clock J2 pin 1 (GND) | |
| 5V | clock J1 pin 1 (+5V) | one shared 5 V rail |
| GPIO32 | KY-040 CLK | scroll knob |
| GPIO33 | KY-040 DT | |
| GPIO25 | KY-040 SW | |
| 3V3 / GND | KY-040 + / GND | power the knob from 3.3 V, not 5 V |

The DS3231 RTC stays wired to the clock board (J2 pins 2/3), and the three back buttons stay on the 32U4.

Power: use one USB cable, into the ESP32. Its 5 V feeds the clock through J1 pin 1, which skips the
thin USB trace on the clock board. Do not plug both USB ports in at the same time; unplug the ESP32 to
flash the 32U4.

## Scroll knob

Same behaviour as the 32U4 knob firmware 0.4.0, now on the ESP32 (`src/input/RotaryKnob.*`, logic in
`HostRuntime::handleKnob`):

| Action | Clock | Timer | Stopwatch |
| --- | --- | --- | --- |
| Turn | brightness | timer length (stopped) / brightness (running) | brightness |
| Click | go to Timer | go to Stopwatch | go to Clock |
| Hold ~1 s, release | night red on/off | start / pause | start / pause |
| Hold 2 s | - | reset | reset |

Night red: at the lowest brightness, pause, then turn down one more click. Turn up to leave it.
If clockwise turns things down, set `kReverse = true` in `src/input/RotaryKnob.h`.

## clockWinder over WiFi

The ESP32 listens on TCP port 3333 and speaks the same `ClockRender/1` text commands the GUI uses over
USB (it applies them to the ESP32 host model). In clockWinder, type `esp32-clock.local` (or the clock's
IP, e.g. `192.168.4.1` when you are on its own hotspot) in the port box and press Connect. Calibration
still needs USB to the clock board.

## Project Layout

- `src/main.cpp` - host startup and main loop
- `src/model/ClockConfig.h` - persisted host config model
- `src/storage/SettingsStore.*` - LittleFS JSON persistence
- `src/render/SegmentFrame.*` - stable logical-target frame model
- `src/render/FrameCompiler.*` - compiles presets/faces/widgets into a renderer frame
- `src/host/LeonardoClient.*` - UART client for `ClockRender/1`
- `src/runtime/HostRuntime.*` - host-owned timer, stopwatch, time source, and renderer orchestration
- `src/web/WebUiServer.*` - HTTP API and static file serving
- `src/net/WinderServer.*` - clockWinder TCP bridge (port 3333)
- `src/input/RotaryKnob.*` - KY-040 scroll knob
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

To join your home WiFi, turn on station mode in the web UI network settings. After that the clock
is at `http://esp32-clock.local/`.

The web UI edits host config and renderer-link settings, then drives the Leonardo renderer over UART.
