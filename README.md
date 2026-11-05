# Modular NeoPixel Smart Clock

A custom 4-digit clock on a KiCad-designed PCB: an **ATmega32U4** (Arduino Leonardo compatible), **31 SK6812MINI addressable LEDs**, 3 buttons, and an external **DS3231 RTC**. It comes with a desktop app, **clockWinder**, for LED mapping, testing, and day-to-day control.

This started as a WPI PCB course project and is being grown into a polished, modular clock platform.

## Repository layout

| Path | What's there |
| --- | --- |
| `hardware/pcb/source (kicad files)/` | KiCad 9 project `NG_WPI_7seg_PCB` (the original course submission is in `OLD NAME PCB SUBMISSION/`) |
| `hardware/pcb/schematic/` | Schematic PDFs |
| `hardware/pcb/fabrication/` | Gerbers and drill files sent to JLCPCB |
| `hardware/pcb/WPI PCB Submission ZIP/` | The zip submitted for the course |
| `firmware/arduino/clock_firmware/` | Main clock firmware |
| `firmware/arduino/clock_calibration_firmware/` | LED mapping / calibration firmware |
| `softwear/clockWinder_gui/` | clockWinder desktop app (PySide6) |
| `docs/` | Digi-Key parts list, architecture write-up |
| `archive/` | The archived WPI starter repo and older generated code packages |

## Hardware

- ATmega32U4 at 16 MHz, micro USB, ICSP header. It flashes as an **Arduino Leonardo**.
- 31 SK6812MINI LEDs on **D5**: four 7-segment digits (28), a 2-dot colon, and a decimal point.
- Buttons on **D8, D9, D10**. They're active low and pull the pin to GND when pressed.
- Board is 102 x 40 mm.

### DS3231 RTC module (external)

The RTC is a hiLetgo DS3231 module wired to the board, not mounted on it:

| DS3231 | Clock board |
| --- | --- |
| VCC | 5V |
| GND | GND |
| SDA | D2 / SDA |
| SCL | D3 / SCL |

Leave SQW and 32K unconnected. The **CR2032 goes in the RTC module** and keeps time when USB power is removed.

## Firmware

There are two sketches. Both need the **Adafruit NeoPixel** library, and the main one also needs **RTClib**.

1. **`clock_calibration_firmware`** is for mapping which physical LED is which segment. Flash it, then use the clockWinder calibration window and save the map to EEPROM.
2. **`clock_firmware`** is the actual clock. It loads the saved map from EEPROM, or falls back to `Generated_LedMap.h`.

To upload, open the sketch's `.ino` in Arduino IDE, choose **Arduino Leonardo** and the right port, and upload. See [`firmware/README.md`](firmware/README.md) for details and the serial protocol.

### Display modes

Clock, Clock with seconds (MM:SS), Timer, Stopwatch, and Color demo.

### Buttons (clock firmware)

| Button | Clock / other modes | Timer mode | Stopwatch mode |
| --- | --- | --- | --- |
| BTN1 (D8) | next mode | next mode | next mode |
| BTN2 (D9) | jump to Clock | +1 minute | start / stop |
| BTN3 (D10) | jump to Color demo | -1 minute | reset |

Only short presses are handled right now. Long-press actions are on the to-do list.

## clockWinder desktop app

```bash
cd softwear/clockWinder_gui
pip install -r requirements.txt
python main.py
```

It connects over USB serial at 115200 baud. The main window sets modes, colors, brightness, the RTC time, the timer, and the stopwatch. The calibration window walks through each physical LED, assigns it a segment, and exports `ledmap.json` / `Generated_LedMap.h`. Saved maps live in `softwear/clockWinder_gui/clockWinder/ledmaps/`. See [`softwear/clockWinder_gui/README.md`](softwear/clockWinder_gui/README.md).

## Good first test sequence

1. Wire the DS3231 and insert the CR2032.
2. Flash `clock_calibration_firmware`, map the LEDs in clockWinder, and save to EEPROM.
3. Flash `clock_firmware`.
4. In clockWinder: connect, **Read RTC**, then **Set RTC from PC time**.
5. Pick Clock mode, adjust colors and brightness, and save settings.

## Troubleshooting

- **Time is wrong after power loss:** the CR2032 is missing or dead, SDA/SCL is miswired, or the RTC was never set.
- **App won't connect:** close the Arduino Serial Monitor and any other serial tool, replug the board, refresh ports, and pick the `/dev/cu.usbmodem...` (macOS) or COM port.

## Roadmap

- Enclosure / 3D printed housing
- Long-press button actions
- Sensors and smarter modes
- Hardware v2 with the RTC on the board
