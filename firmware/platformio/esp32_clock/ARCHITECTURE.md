# ESP32 Host Architecture

This project is the high-level host half of a two-MCU system.

## Core Idea

The ESP32 owns the evolving product model. The Leonardo owns the stable board-rendering contract.

New product features should normally be added by:

1. extending host config and storage
2. exposing the feature in the web/API layer
3. compiling the result into a stable render frame or compact renderer command

That keeps the Leonardo simple while letting the ESP32 change freely.

## Active Layers

### `src/model`

- `src/model/ClockConfig.h`

This is the host-side persisted product model. The public config now centers on:

- `network`
- `rendererLink`
- `time`
- `runtime`
- `presets`
- `widgets`
- `faces`

Legacy direct-drive hardware and mapping fields remain only for backward config compatibility and are no longer the primary model.

### `src/storage`

- `src/storage/SettingsStore.*`

Persists the host model to LittleFS and performs config import/export.

### `src/render`

- `src/render/SegmentFrame.*`
- `src/render/FrameCompiler.*`

This layer is the stability boundary.

- `SegmentFrame` is the compact logical-target render model
- `FrameCompiler` turns high-level presets, faces, widgets, and runtime state into that frame

Because the ESP32 compiles to this intermediate form, future features can grow without teaching the Leonardo what each feature means.

### `src/host`

- `src/host/LeonardoClient.*`

This is the UART transport/client for `ClockRender/1`.

Responsibilities:

- hello/status/capability probing
- renderer RTC and map reads
- host/local control negotiation
- async event handling
- `FRAME24` transport

### `src/runtime`

- `src/runtime/HostRuntime.*`

This is the orchestration layer for the host platform.

Responsibilities:

- host-owned timer and stopwatch behavior
- time-source priority `NTP -> renderer RTC -> fallback`
- periodic RTC sync back to the renderer
- renderer health polling and control refresh
- button event interpretation
- render scheduling and frame submission

### `src/web`

- `src/web/WebUiServer.*`

Thin HTTP/API layer over the host runtime and config store.

### `data`

- `data/index.html`
- `data/app.css`
- `data/app.js`

The browser UI edits host config and renderer-link settings, shows renderer health, and triggers service actions. It is no longer a mapping editor for the custom board.

## Control Flow

1. ESP32 boots and loads host config from LittleFS.
2. `LeonardoClient` opens UART2 using the configured renderer link.
3. `HostRuntime` discovers the renderer and requests host control.
4. Host state is compiled into a `SegmentFrame`.
5. The frame is sent as `FRAME24`.
6. Leonardo renders the frame using its own logical-to-physical LED map.

## Calibration Boundary

Calibration is intentionally outside the normal ESP32 host flow.

- physical mapping edits belong to the Leonardo calibration sketch
- the ESP32 host may query and export the renderer map
- the ESP32 host should not become the primary writer of board calibration data

## Design Rules

- do not reintroduce direct LED driving on the ESP32 primary path
- keep web actions thin and runtime-driven
- treat `SegmentFrame` and `ClockRender/1` as the stable renderer contract
- keep feature growth on the host side whenever possible
