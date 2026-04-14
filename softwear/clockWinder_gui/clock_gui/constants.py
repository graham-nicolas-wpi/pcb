from __future__ import annotations

from enum import IntEnum
from pathlib import Path

APP_NAME = "NeoPixel Clock Console"
SERIAL_BAUDRATE = 115200
SERIAL_POLL_INTERVAL_MS = 20
SERIAL_HANDSHAKE_DELAY_MS = 450
DEFAULT_NUM_PIXELS = 31
DEFAULT_PIXEL_PIN = 5
LOG_UNUSED = 255
PACKAGE_ROOT = Path(__file__).resolve().parent
DEFAULT_EXPORT_DIR = PACKAGE_ROOT.parent
JSON_SCHEMA = "neopixel-clock-map/v1"


class LogicalId(IntEnum):
    D1_A = 0
    D1_B = 1
    D1_C = 2
    D1_D = 3
    D1_E = 4
    D1_F = 5
    D1_G = 6
    D2_A = 7
    D2_B = 8
    D2_C = 9
    D2_D = 10
    D2_E = 11
    D2_F = 12
    D2_G = 13
    COLON_TOP = 14
    COLON_BOTTOM = 15
    D3_A = 16
    D3_B = 17
    D3_C = 18
    D3_D = 19
    D3_E = 20
    D3_F = 21
    D3_G = 22
    D4_A = 23
    D4_B = 24
    D4_C = 25
    D4_D = 26
    D4_E = 27
    D4_F = 28
    D4_G = 29
    DECIMAL = 30


MODES = ["CLOCK", "CLOCK_SECONDS", "TIMER", "STOPWATCH", "COLOR_DEMO"]
HOUR_MODES = ["24H", "12H"]
SECONDS_MODES = ["BLINK", "ON", "OFF"]
TIMER_FORMATS = [
    ("Auto", "AUTO"),
    ("HH:MM", "HHMM"),
    ("MM:SS", "MMSS"),
    ("SS.CS", "SSCS"),
]

BASE_COLOR_TARGETS = [
    "CLOCK",
    "HOURS",
    "MINUTES",
    "DIGIT1",
    "DIGIT2",
    "DIGIT3",
    "DIGIT4",
    "ACCENT",
    "SECONDS",
    "TIMER",
]
TIMER_COLOR_TARGETS = ["TIMER_HOURS", "TIMER_MINUTES", "TIMER_SECONDS"]
STOPWATCH_COLOR_TARGETS = [
    "STOPWATCH_HOURS",
    "STOPWATCH_MINUTES",
    "STOPWATCH_SECONDS",
]
COLOR_TARGETS = BASE_COLOR_TARGETS + TIMER_COLOR_TARGETS + STOPWATCH_COLOR_TARGETS

COLOR_PRESETS = {
    "Warm Classic": {
        "CLOCK": (255, 120, 40),
        "HOURS": (255, 120, 40),
        "MINUTES": (255, 120, 40),
        "DIGIT1": (255, 120, 40),
        "DIGIT2": (255, 120, 40),
        "DIGIT3": (255, 120, 40),
        "DIGIT4": (255, 120, 40),
        "ACCENT": (0, 120, 255),
        "SECONDS": (255, 255, 255),
        "TIMER": (80, 255, 120),
        "TIMER_HOURS": (80, 255, 120),
        "TIMER_MINUTES": (80, 255, 120),
        "TIMER_SECONDS": (80, 255, 120),
        "STOPWATCH_HOURS": (255, 120, 40),
        "STOPWATCH_MINUTES": (255, 120, 40),
        "STOPWATCH_SECONDS": (255, 255, 255),
        "preset_cmd": None,
    },
    "Ice Blue": {
        "CLOCK": (120, 220, 255),
        "HOURS": (120, 220, 255),
        "MINUTES": (120, 220, 255),
        "DIGIT1": (120, 220, 255),
        "DIGIT2": (120, 220, 255),
        "DIGIT3": (120, 220, 255),
        "DIGIT4": (120, 220, 255),
        "ACCENT": (0, 80, 255),
        "SECONDS": (220, 245, 255),
        "TIMER": (80, 255, 220),
        "TIMER_HOURS": (80, 255, 220),
        "TIMER_MINUTES": (80, 255, 220),
        "TIMER_SECONDS": (80, 255, 220),
        "STOPWATCH_HOURS": (120, 220, 255),
        "STOPWATCH_MINUTES": (120, 220, 255),
        "STOPWATCH_SECONDS": (220, 245, 255),
        "preset_cmd": None,
    },
    "Matrix": {
        "CLOCK": (0, 255, 80),
        "HOURS": (0, 255, 80),
        "MINUTES": (0, 255, 80),
        "DIGIT1": (0, 255, 80),
        "DIGIT2": (0, 255, 80),
        "DIGIT3": (0, 255, 80),
        "DIGIT4": (0, 255, 80),
        "ACCENT": (0, 120, 40),
        "SECONDS": (180, 255, 180),
        "TIMER": (80, 255, 120),
        "TIMER_HOURS": (80, 255, 120),
        "TIMER_MINUTES": (80, 255, 120),
        "TIMER_SECONDS": (80, 255, 120),
        "STOPWATCH_HOURS": (0, 255, 80),
        "STOPWATCH_MINUTES": (0, 255, 80),
        "STOPWATCH_SECONDS": (180, 255, 180),
        "preset_cmd": None,
    },
    "Sunset": {
        "CLOCK": (255, 90, 40),
        "HOURS": (255, 90, 40),
        "MINUTES": (255, 90, 40),
        "DIGIT1": (255, 90, 40),
        "DIGIT2": (255, 90, 40),
        "DIGIT3": (255, 90, 40),
        "DIGIT4": (255, 90, 40),
        "ACCENT": (255, 0, 120),
        "SECONDS": (255, 220, 120),
        "TIMER": (255, 180, 80),
        "TIMER_HOURS": (255, 180, 80),
        "TIMER_MINUTES": (255, 180, 80),
        "TIMER_SECONDS": (255, 180, 80),
        "STOPWATCH_HOURS": (255, 90, 40),
        "STOPWATCH_MINUTES": (255, 90, 40),
        "STOPWATCH_SECONDS": (255, 220, 120),
        "preset_cmd": None,
    },
    "Rainbow": {
        "CLOCK": (255, 0, 0),
        "HOURS": (255, 60, 0),
        "MINUTES": (0, 120, 255),
        "DIGIT1": (255, 0, 0),
        "DIGIT2": (255, 120, 0),
        "DIGIT3": (0, 255, 0),
        "DIGIT4": (0, 120, 255),
        "ACCENT": (180, 0, 255),
        "SECONDS": (0, 120, 255),
        "TIMER": (180, 0, 255),
        "TIMER_HOURS": (180, 0, 255),
        "TIMER_MINUTES": (180, 0, 255),
        "TIMER_SECONDS": (180, 0, 255),
        "STOPWATCH_HOURS": (255, 0, 0),
        "STOPWATCH_MINUTES": (255, 120, 0),
        "STOPWATCH_SECONDS": (0, 120, 255),
        "preset_cmd": "PRESET RAINBOW",
    },
    "Party": {
        "CLOCK": (255, 0, 255),
        "HOURS": (255, 0, 255),
        "MINUTES": (255, 0, 255),
        "DIGIT1": (255, 0, 255),
        "DIGIT2": (255, 0, 255),
        "DIGIT3": (255, 0, 255),
        "DIGIT4": (255, 0, 255),
        "ACCENT": (0, 255, 255),
        "SECONDS": (255, 255, 0),
        "TIMER": (255, 80, 80),
        "TIMER_HOURS": (255, 80, 80),
        "TIMER_MINUTES": (255, 80, 80),
        "TIMER_SECONDS": (255, 80, 80),
        "STOPWATCH_HOURS": (255, 0, 255),
        "STOPWATCH_MINUTES": (255, 0, 255),
        "STOPWATCH_SECONDS": (255, 255, 0),
        "preset_cmd": "PRESET PARTY",
    },
}


def logical_name(value: int) -> str:
    if value == LOG_UNUSED:
        return "UNUSED"
    try:
        return LogicalId(value).name
    except ValueError:
        return f"INVALID({value})"


def logical_picker_label(value: int) -> str:
    if value == LOG_UNUSED:
        return "UNUSED"
    return f"{LogicalId(value).name} ({value})"
