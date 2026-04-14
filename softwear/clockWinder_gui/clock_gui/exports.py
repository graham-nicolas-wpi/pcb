from __future__ import annotations

import json
from pathlib import Path

from .constants import LOG_UNUSED
from .model import MappingModel


def mapping_to_json_text(model: MappingModel) -> str:
    return json.dumps(model.to_json_obj(), indent=2)


def mapping_to_header_text(model: MappingModel) -> str:
    logical_to_phys = model.logical_to_phys()

    def array(values: list[int]) -> str:
        return "{ " + ", ".join(str(value) for value in values) + " }"

    return "\n".join(
        [
            "#pragma once",
            "#include <stdint.h>",
            '#include "LogicalIds.h"',
            "",
            f"constexpr uint8_t kPhysToLogical[kNumPixels] = {array(model.phys_to_logical)};",
            f"constexpr uint8_t kLogicalToPhys[LOGICAL_COUNT] = {array(logical_to_phys)};",
            "",
        ]
    )


def save_mapping_json(path: str | Path, model: MappingModel) -> None:
    Path(path).write_text(mapping_to_json_text(model), encoding="utf-8")


def save_mapping_header(path: str | Path, model: MappingModel) -> None:
    Path(path).write_text(mapping_to_header_text(model), encoding="utf-8")


def load_mapping_json(path: str | Path) -> MappingModel:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    return MappingModel.from_json_obj(data)


def is_mapping_complete(model: MappingModel) -> bool:
    return not model.validate() and all(value != LOG_UNUSED for value in model.phys_to_logical)
