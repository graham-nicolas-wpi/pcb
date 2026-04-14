# Leonardo Renderer + ESP32 Host

This document is the canonical architecture and protocol contract for the long-term no-PCB-redesign clock stack.

## Hardware Plan

The current custom PCB remains unchanged.

- Leonardo stays permanently installed on the custom board
- Leonardo keeps ownership of the onboard NeoPixel data path, board buttons, and DS3231 path
- ESP32-WROOM-32E is an external host connected through exposed headers only
- the main plan does not involve rewiring `LED_DIN`, soldering to hidden traces, or replacing the Leonardo on this board revision

Recommended UART wiring:

- ESP32 `GPIO17 TX` -> Leonardo `D0 / RX`
- Leonardo `D1 / TX` -> ESP32 `GPIO16 RX` through level protection
- common `GND`
- share power through the accessible headers as appropriate for the build

## Role Split

### Leonardo Renderer

The Leonardo is the board-native rendering appliance.

- renders logical digits, colon, and decimal
- preserves the physical LED map abstraction
- stores and uses the authoritative calibration map
- reads board buttons and RTC
- offers a stable serial control surface
- falls back to local clock/timer/stopwatch behavior when the host is absent or released

### ESP32 Host

The ESP32 is the flexible system brain.

- owns presets, faces, widgets, schedules, storage, and network policy
- owns NTP and other future time sources
- serves the browser UI and HTTP API
- compiles high-level product features into stable renderer commands
- should absorb most future feature growth without requiring Leonardo firmware changes

## Protocol Families

- `ClockRender/1` - Leonardo runtime protocol
- `ClockCal/1` - Leonardo calibration firmware protocol
- `Clock/1` - legacy runtime hello alias kept for migration compatibility

`ClockCal/1` belongs to the calibration sketch only. The normal Leonardo runtime is canonical on `ClockRender/1`.

## Transport

- UART
- `115200` baud by default
- `8N1`
- newline-delimited ASCII commands

## Runtime Control Model

The renderer has two control states:

- `CONTROL LOCAL` - Leonardo standalone behavior is active
- `CONTROL HOST` - ESP32 host is authoritative

In host mode:

- Leonardo buttons no longer mutate renderer state locally
- button presses are surfaced as events when `BTNREPORT 1` is enabled
- the ESP32 translates those events into product behavior

If host traffic stops for roughly 5 seconds, Leonardo returns to local control and emits `EV HOST TIMEOUT`.

## Core Commands

### Session and discovery

- `HELLO`
- `HELLO ClockRender/1`
- `HELLO Clock/1`
- `INFO?`
- `CAPS?`
- `STATUS?`
- `PING`

### Control and board services

- `CONTROL HOST`
- `CONTROL LOCAL`
- `BTNREPORT 0|1`
- `RTC READ`
- `RTC SET yyyy mm dd hh mm ss`
- `MAP?`
- `VALIDATE?`
- `TEST DIGITS`
- `TEST SEGMENTS`
- `TEST ALL`

### Convenience renderer commands

- `MODE CLOCK|CLOCK_SECONDS|TIMER|STOPWATCH|COLOR_DEMO`
- `BRIGHTNESS n`
- `COLONBRIGHTNESS n`
- `COLOR <target> r g b`
- `TIMER SET seconds`
- `TIMER START`
- `TIMER STOP`
- `TIMER RESET`
- `STOPWATCH START`
- `STOPWATCH STOP`
- `STOPWATCH RESET`

These are useful stable controls, but the long-term extensibility lane is the frame contract below.

## Stable Frame Contract

`FRAME24 <payload>`

- payload length: `186` hex characters
- content: `31` logical targets, each encoded as `RRGGBB`
- target order: `LogicalIds.h` order on the Leonardo runtime
- semantic level: logical display targets, not physical pixel indices

That means the ESP32 can invent richer faces, widgets, themes, schedules, and integrations, then reduce them to a `SegmentFrame` without teaching the Leonardo what each new feature means.

## Events

The Leonardo runtime may emit asynchronous events:

- `EV BUTTON BTN1 SHORT`
- `EV BUTTON BTN2 SHORT`
- `EV BUTTON BTN3 SHORT`
- `EV TIMER DONE`
- `EV HOST TIMEOUT`

## Calibration Boundary

Calibration stays Leonardo-owned.

- use `firmware/arduino/clock_calibration_firmware/` to write or repair the LED map
- use `ClockCal/1` for the calibration workflow
- the normal renderer runtime may report mapping and validation state, but it is not the primary map-editing tool

## How To Add Future Features

For most new ideas:

1. add config, storage, UI, and behavior on the ESP32
2. compile that behavior into convenience commands or `FRAME24`
3. keep the Leonardo unchanged unless the stable rendering contract truly needs a new primitive

This keeps the Leonardo durable and lets the ESP32 architecture evolve freely.
