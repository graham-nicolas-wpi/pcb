# Scroll knob (KY-040 rotary encoder) — firmware 0.4.0

## Wiring

| KY-040 pin | Clock board pin | Chip pin |
|---|---|---|
| CLK | bottom header **J2 pin 5** | D0 (PD2, INT2) |
| DT  | bottom header **J2 pin 4** | D1 (PD3, INT3) |
| SW  | bottom header **J2 pin 6** | D11 (PB7) |
| +   | ICSP header **J3 pin 2** (5V), or share top header J1 pin 1 with the RTC | 5V |
| GND | top header **J1 pin 3** (or ICSP J3 pin 6) | GND |

J2 pin 1 is the square pad (GND); pins 2 and 3 already go to the RTC.
D0/D1 are the hardware UART pins, which this firmware does not use (USB serial is separate), and both
have their own interrupts so fast spins are not missed. The JTAG pins (A0–A3) are not needed.

If turning clockwise makes it dimmer, set `kReverse = true` in `src/hal/RotaryEncoder.h`
(or swap the CLK and DT wires).

## What it does

| Action | Clock | Timer | Stopwatch |
|---|---|---|---|
| Turn | brightness | sets the timer length (while stopped) / brightness (while running) | brightness |
| Click | go to Timer | go to Stopwatch | go to Clock |
| Hold ~1 s, release | night red on/off | start / pause | start / pause |
| Hold 2 s | – | reset to the set length | reset to 0 |

- **Brightness** has 16 steps from 6 to 255 and is saved like the GUI setting. The colon now dims with it.
- **Night red:** turn down to the lowest step and the knob stops there. Let go for 0.7 s, then turn down one
  more click: every lit segment turns pure red at level 1. Turn up one click to go back to normal (lowest step).
  Night red is remembered through power loss.
- **Timer steps:** 5 s up to 1 min, 30 s up to 10 min, 1 min up to 1 h, then 5 min, max 12 h.
  Turning while paused resets the timer to the new length.
- The three buttons on the back still work as before.

Tunables are at the top of the `Knob` namespace in `Clock_Firmware.ino` (`kLevels`, `kNightPauseMs`,
`kNightRedLevel`) and in `RotaryEncoder.h` (`kStepsPerDetent`, `kLongPressMs`, `kHoldMs`).
