from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any

from .constants import DEFAULT_NUM_PIXELS, DEFAULT_PIXEL_PIN, JSON_SCHEMA, LOG_UNUSED, LogicalId, logical_name


@dataclass
class MappingModel:
    num_pixels: int = DEFAULT_NUM_PIXELS
    pixel_pin: int = DEFAULT_PIXEL_PIN
    color_order: str = "GRB"
    phys_to_logical: list[int] = field(
        default_factory=lambda: [LOG_UNUSED] * DEFAULT_NUM_PIXELS
    )

    def clear(self) -> None:
        self.phys_to_logical = [LOG_UNUSED] * self.num_pixels

    def assign(self, phys: int, logical: int) -> None:
        if not 0 <= phys < self.num_pixels:
            raise ValueError(f"Physical index out of range: {phys}")
        if logical != LOG_UNUSED and logical not in {item.value for item in LogicalId}:
            raise ValueError(f"Logical id out of range: {logical}")
        self.phys_to_logical[phys] = logical

    def unassign(self, phys: int) -> None:
        self.assign(phys, LOG_UNUSED)

    def apply_map(self, values: list[int]) -> None:
        if len(values) != self.num_pixels:
            raise ValueError(
                f"Expected {self.num_pixels} mapping values, received {len(values)}"
            )
        self.phys_to_logical = list(values)

    def used_logicals(self) -> dict[int, list[int]]:
        used: dict[int, list[int]] = {}
        for phys, logical in enumerate(self.phys_to_logical):
            if logical == LOG_UNUSED:
                continue
            used.setdefault(logical, []).append(phys)
        return used

    def duplicate_logicals(self) -> dict[int, list[int]]:
        return {
            logical: phys_list
            for logical, phys_list in self.used_logicals().items()
            if len(phys_list) > 1
        }

    def missing_logicals(self) -> list[int]:
        assigned = set(self.used_logicals())
        return sorted(
            logical.value for logical in LogicalId if logical.value not in assigned
        )

    def validate(self) -> list[str]:
        errors: list[str] = []
        duplicates = self.duplicate_logicals()
        if duplicates:
            for logical, phys_list in sorted(duplicates.items()):
                errors.append(
                    f"Duplicate assignment for {logical_name(logical)} on physical LEDs {phys_list}"
                )
        missing = self.missing_logicals()
        if missing:
            errors.append(
                f"Missing {len(missing)} logical targets: "
                + ", ".join(logical_name(value) for value in missing[:6])
                + ("..." if len(missing) > 6 else "")
            )
        return errors

    def logical_to_phys(self) -> list[int]:
        values = [LOG_UNUSED] * len(LogicalId)
        for phys, logical in enumerate(self.phys_to_logical):
            if logical != LOG_UNUSED and 0 <= logical < len(values):
                values[logical] = phys
        return values

    def summary(self) -> str:
        assigned = sum(1 for value in self.phys_to_logical if value != LOG_UNUSED)
        duplicates = len(self.duplicate_logicals())
        missing = len(self.missing_logicals())
        return (
            f"Assigned {assigned}/{self.num_pixels} LEDs, "
            f"{missing} missing, {duplicates} duplicate groups"
        )

    def logical_for_phys(self, phys: int) -> int:
        if not 0 <= phys < self.num_pixels:
            return LOG_UNUSED
        return self.phys_to_logical[phys]

    def to_json_obj(self) -> dict[str, Any]:
        return {
            "schema": JSON_SCHEMA,
            "num_pixels": self.num_pixels,
            "pixel_pin": self.pixel_pin,
            "color_order": self.color_order,
            "phys_to_logical": [
                logical_name(value) if value != LOG_UNUSED else "UNUSED"
                for value in self.phys_to_logical
            ],
            "notes": {"digit_order": "D1 D2 : D3 D4"},
        }

    @classmethod
    def from_json_obj(cls, data: dict[str, Any]) -> "MappingModel":
        num_pixels = int(data.get("num_pixels", DEFAULT_NUM_PIXELS))
        pixel_pin = int(data.get("pixel_pin", DEFAULT_PIXEL_PIN))
        color_order = str(data.get("color_order", "GRB"))
        raw_values = list(data.get("phys_to_logical", []))
        if len(raw_values) != num_pixels:
            raise ValueError("JSON mapping does not match the declared pixel count")

        name_to_value = {item.name: int(item.value) for item in LogicalId}
        phys_to_logical: list[int] = []
        for raw in raw_values:
            if raw in ("UNUSED", LOG_UNUSED):
                phys_to_logical.append(LOG_UNUSED)
                continue
            if isinstance(raw, int):
                phys_to_logical.append(raw)
                continue
            if str(raw) not in name_to_value:
                raise ValueError(f"Unknown logical name in JSON: {raw}")
            phys_to_logical.append(name_to_value[str(raw)])

        return cls(
            num_pixels=num_pixels,
            pixel_pin=pixel_pin,
            color_order=color_order,
            phys_to_logical=phys_to_logical,
        )


@dataclass
class DeviceHello:
    protocol: str = ""
    name: str = ""
    firmware_version: str = ""
    pixels: int = DEFAULT_NUM_PIXELS
    logical_count: int = len(LogicalId)
    calibration_mode: bool = False
    rtc_present: bool | None = None
    board: str = ""
    pin: str = ""
    buttons: str = ""


@dataclass
class DeviceStatus:
    mode: str = "CLOCK"
    hour_mode: str = "24H"
    seconds_mode: str = "BLINK"
    timer_format: str = "AUTO"
    brightness: int = 64
    colon_brightness: int = 255
    rainbow: bool = False
    timer_preset: int = 300
    timer_remaining: int = 300
    timer_running: bool = False
    stopwatch_running: bool = False
    calibration_mode: bool = False


@dataclass
class DeviceRtc:
    summary: str = "unknown"
    lost_power: bool | None = None
    is_missing: bool = False
