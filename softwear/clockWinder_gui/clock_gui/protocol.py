from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .constants import LOG_UNUSED


def _parse_key_values(tokens: list[str]) -> dict[str, str]:
    values: dict[str, str] = {}
    for token in tokens:
        if "=" not in token:
            continue
        key, value = token.split("=", 1)
        values[key] = value
    return values


def _int_value(values: dict[str, str], key: str, default: int) -> int:
    try:
        return int(values.get(key, default))
    except (TypeError, ValueError):
        return default


def _bool_value(values: dict[str, str], key: str, default: bool = False) -> bool:
    raw = values.get(key)
    if raw is None:
        return default
    return raw not in {"0", "false", "False", ""}


@dataclass
class ParsedLine:
    kind: str
    raw: str
    data: dict[str, Any]


class ClockProtocol:
    @staticmethod
    def hello(protocol: str = "ClockRender/1") -> str:
        return f"HELLO {protocol}"

    @staticmethod
    def info() -> str:
        return "INFO?"

    @staticmethod
    def status() -> str:
        return "STATUS?"

    @staticmethod
    def current() -> str:
        return "CUR?"

    @staticmethod
    def mapping() -> str:
        return "MAP?"

    @staticmethod
    def validate() -> str:
        return "VALIDATE?"

    @staticmethod
    def mode_cal() -> str:
        return "MODE CAL"

    @staticmethod
    def mode_run() -> str:
        return "MODE RUN"

    @staticmethod
    def prev() -> str:
        return "PREV"

    @staticmethod
    def next() -> str:
        return "NEXT"

    @staticmethod
    def set_current(phys: int) -> str:
        return f"SET {phys}"

    @staticmethod
    def assign(logical: int) -> str:
        return f"ASSIGN {logical}"

    @staticmethod
    def unassign_current() -> str:
        return "UNASSIGN_CUR"

    @staticmethod
    def save_map() -> str:
        return "SAVE"

    @staticmethod
    def load_map() -> str:
        return "LOAD"

    @staticmethod
    def test_segments() -> str:
        return "TEST SEGMENTS"

    @staticmethod
    def test_digits() -> str:
        return "TEST DIGITS"

    @staticmethod
    def test_all() -> str:
        return "TEST ALL"

    @staticmethod
    def set_mode(mode: str) -> str:
        return f"MODE {mode}"

    @staticmethod
    def set_hour_mode(mode: str) -> str:
        return f"HOURMODE {mode}"

    @staticmethod
    def set_seconds_mode(mode: str) -> str:
        return f"SECONDSMODE {mode}"

    @staticmethod
    def set_timer_format(format_name: str) -> str:
        return f"TIMERFORMAT {format_name}"

    @staticmethod
    def set_brightness(value: int) -> str:
        return f"BRIGHTNESS {value}"

    @staticmethod
    def set_colon_brightness(value: int) -> str:
        return f"COLONBRIGHTNESS {value}"

    @staticmethod
    def set_color(target: str, rgb: tuple[int, int, int]) -> str:
        red, green, blue = rgb
        return f"COLOR {target} {red} {green} {blue}"

    @staticmethod
    def preset(name: str) -> str:
        return f"PRESET {name}"

    @staticmethod
    def rtc_read() -> str:
        return "RTC READ"

    @staticmethod
    def rtc_set(year: int, month: int, day: int, hour: int, minute: int, second: int) -> str:
        return (
            f"RTC SET {year:04d} {month:02d} {day:02d} "
            f"{hour:02d} {minute:02d} {second:02d}"
        )

    @staticmethod
    def timer_set(seconds: int) -> str:
        return f"TIMER SET {seconds}"

    @staticmethod
    def timer_start() -> str:
        return "TIMER START"

    @staticmethod
    def timer_stop() -> str:
        return "TIMER STOP"

    @staticmethod
    def timer_reset() -> str:
        return "TIMER RESET"

    @staticmethod
    def stopwatch_start() -> str:
        return "STOPWATCH START"

    @staticmethod
    def stopwatch_stop() -> str:
        return "STOPWATCH STOP"

    @staticmethod
    def stopwatch_reset() -> str:
        return "STOPWATCH RESET"

    @staticmethod
    def parse_line(line: str) -> ParsedLine:
        text = line.strip()
        if not text:
            return ParsedLine(kind="empty", raw=line, data={})

        if text.startswith("OK HELLO "):
            parts = text.split()
            data = _parse_key_values(parts[3:])
            data["protocol"] = parts[2] if len(parts) > 2 else ""
            data["calibration_mode"] = _bool_value(data, "CAL")
            data["rtc_present"] = None if "RTC" not in data else _bool_value(data, "RTC")
            data["pixels"] = _int_value(data, "PIXELS", 31)
            data["logical"] = _int_value(data, "LOGICAL", 31)
            return ParsedLine(kind="hello", raw=text, data=data)

        if text.startswith("OK INFO "):
            return ParsedLine(kind="info", raw=text, data=_parse_key_values(text.split()[2:]))

        if text.startswith("OK STATUS "):
            data = _parse_key_values(text.split()[2:])
            data["current_phys"] = _int_value(data, "CUR", 0)
            if "MAP_VALID" in data:
                data["mapping_valid"] = _bool_value(data, "MAP_VALID")
            elif "VALID" in data:
                data["mapping_valid"] = _bool_value(data, "VALID")
            else:
                data["mapping_valid"] = None
            data["brightness"] = _int_value(data, "BRIGHTNESS", 64)
            data["colon_brightness"] = _int_value(data, "COLONBRIGHTNESS", 255)
            data["timer_preset"] = _int_value(data, "TIMER_PRESET", 300)
            data["timer_remaining"] = _int_value(data, "TIMER_REMAINING", 300)
            data["timer_format"] = data.get("TIMERFORMAT", "AUTO")
            data["timer_running"] = _bool_value(data, "TIMER_RUNNING")
            data["stopwatch_running"] = _bool_value(data, "STOPWATCH_RUNNING")
            data["rainbow"] = _bool_value(data, "RAINBOW")
            data["calibration_mode"] = _bool_value(data, "CAL")
            data["control"] = data.get("CONTROL", "LOCAL")
            data["frame"] = data.get("FRAME", "LOCAL")
            return ParsedLine(kind="status", raw=text, data=data)

        if text.startswith("CUR "):
            try:
                current_phys = int(text.split()[1])
            except (IndexError, ValueError):
                current_phys = 0
            return ParsedLine(kind="current", raw=text, data={"current_phys": current_phys})

        if text.startswith("MAP "):
            values: list[int] = []
            for item in text[4:].split(","):
                item = item.strip()
                if not item:
                    continue
                try:
                    values.append(int(item))
                except ValueError:
                    values.append(LOG_UNUSED)
            return ParsedLine(kind="map", raw=text, data={"values": values})

        if text.startswith("RTC STATUS MISSING"):
            return ParsedLine(
                kind="rtc",
                raw=text,
                data={"summary": "missing", "is_missing": True, "lost_power": None},
            )

        if text.startswith("RTC "):
            lost_power = None
            if " LOST_POWER=" in text:
                payload, lost_part = text.rsplit(" LOST_POWER=", 1)
                text_value = payload[4:]
                lost_power = lost_part.strip() == "1"
            else:
                text_value = text[4:]
            return ParsedLine(
                kind="rtc",
                raw=text,
                data={
                    "summary": text_value,
                    "is_missing": False,
                    "lost_power": lost_power,
                },
            )

        if text.startswith("EV "):
            if text == "EV TIMER DONE":
                return ParsedLine(kind="event", raw=text, data={"event": "timer_done"})
            if text == "EV HOST TIMEOUT":
                return ParsedLine(kind="event", raw=text, data={"event": "host_timeout"})
            if text.startswith("EV BUTTON "):
                parts = text.split()
                return ParsedLine(
                    kind="event",
                    raw=text,
                    data={
                        "event": "button",
                        "button": parts[2] if len(parts) > 2 else "",
                        "press": parts[3] if len(parts) > 3 else "",
                    },
                )
            return ParsedLine(kind="event", raw=text, data={"event": text[3:]})

        if text.startswith("ERR "):
            return ParsedLine(kind="error", raw=text, data={"message": text[4:]})

        if text.startswith("OK "):
            return ParsedLine(kind="ok", raw=text, data={"message": text[3:]})

        return ParsedLine(kind="text", raw=text, data={"message": text})
