#pragma once

#include <Arduino.h>

#include "SettingsStore.h"

bool applyPresetByName(const char* presetName, ClockSettings& settings);
bool applyColorTarget(const char* target, const RgbColor& newColor, ClockSettings& settings);
