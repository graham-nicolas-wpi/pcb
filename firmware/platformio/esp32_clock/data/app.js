const state = {
  info: null,
  status: null,
  rtc: null,
  network: null,
  metadata: null,
  config: null,
  selectedPresetIndex: 0,
  selectedFaceIndex: 0,
  selectedWidgetIndex: 0,
  logs: [],
};

const q = (id) => document.getElementById(id);

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function log(message) {
  const stamp = new Date().toLocaleTimeString();
  state.logs.unshift(`${stamp}  ${message}`);
  state.logs = state.logs.slice(0, 60);
  const panel = q("logPanel");
  panel.innerHTML = state.logs.map((line) => `<div class="log-line">${line}</div>`).join("");
}

async function getJson(url) {
  const response = await fetch(url);
  if (!response.ok) {
    throw new Error(`GET ${url} failed with ${response.status}`);
  }
  return response.json();
}

async function postJson(url, payload) {
  const response = await fetch(url, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(payload),
  });
  const data = await response.json();
  if (!response.ok || data.ok === false) {
    throw new Error(data.message || `POST ${url} failed`);
  }
  return data;
}

function toHex(color) {
  const value = (channel) => Number(channel || 0).toString(16).padStart(2, "0");
  return `#${value(color.r)}${value(color.g)}${value(color.b)}`;
}

function fromHex(hex) {
  return {
    r: parseInt(hex.slice(1, 3), 16),
    g: parseInt(hex.slice(3, 5), 16),
    b: parseInt(hex.slice(5, 7), 16),
  };
}

function ensureSelectionBounds() {
  if (!state.config) return;
  state.selectedPresetIndex = Math.min(
    state.selectedPresetIndex,
    Math.max(0, state.config.presets.length - 1),
  );
  state.selectedFaceIndex = Math.min(
    state.selectedFaceIndex,
    Math.max(0, state.config.faces.length - 1),
  );
  state.selectedWidgetIndex = Math.min(
    state.selectedWidgetIndex,
    Math.max(0, state.config.widgets.length - 1),
  );
}

function renderStatusRibbon() {
  if (!state.status || !state.network || !state.info) return;
  q("statusRibbon").innerHTML = [
    ["Mode", state.status.mode],
    ["Preset", state.status.activePresetId],
    ["Face", state.status.activeFaceId],
    ["Timer", `${state.status.timerRemainingSeconds}s ${state.status.timerRunning ? "running" : "idle"}`],
    ["Stopwatch", state.status.stopwatchRunning ? "running" : "idle"],
    ["RTC", state.rtc?.present ? state.rtc.summary : "missing"],
    ["AP", state.network.apIp || "192.168.4.1"],
    ["Board", state.info.board],
  ]
    .map(
      ([label, value]) =>
        `<div class="status-pill"><strong>${label}</strong><span>${value}</span></div>`,
    )
    .join("");
}

function fillSelect(select, values, currentValue, labeler = (value) => value) {
  select.innerHTML = values
    .map(
      (value) =>
        `<option value="${value}" ${value === currentValue ? "selected" : ""}>${labeler(
          value,
        )}</option>`,
    )
    .join("");
}

function renderRuntimeControls() {
  if (!state.config || !state.metadata) return;
  fillSelect(q("modeSelect"), state.metadata.modes, state.config.runtime.mode);
  fillSelect(q("hourModeSelect"), state.metadata.hourModes, state.config.runtime.hourMode);
  fillSelect(q("secondsModeSelect"), state.metadata.secondsModes, state.config.runtime.secondsMode);
  fillSelect(q("timerFormatSelect"), state.metadata.timerFormats, state.config.runtime.timerFormat);
  fillSelect(q("effectSelect"), state.metadata.effects, state.config.runtime.effect);

  fillSelect(
    q("activePresetSelect"),
    state.config.presets.map((preset) => preset.id),
    state.config.runtime.activePresetId,
    (id) => state.config.presets.find((preset) => preset.id === id)?.name || id,
  );
  fillSelect(
    q("activeFaceSelect"),
    state.config.faces.map((face) => face.id),
    state.config.runtime.activeFaceId,
    (id) => state.config.faces.find((face) => face.id === id)?.name || id,
  );

  q("brightnessRange").value = state.config.runtime.brightness;
  q("brightnessValue").textContent = state.config.runtime.brightness;
  q("colonBrightnessRange").value = state.config.runtime.colonBrightness;
  q("colonBrightnessValue").textContent = state.config.runtime.colonBrightness;
  q("rainbowEnabledCheckbox").checked = !!state.config.runtime.rainbowEnabled;
}

function renderPresetEditor() {
  ensureSelectionBounds();
  const preset = state.config.presets[state.selectedPresetIndex];
  fillSelect(
    q("presetSelect"),
    state.config.presets.map((item, index) => String(index)),
    String(state.selectedPresetIndex),
    (value) => state.config.presets[Number(value)]?.name || value,
  );

  const colorFields = [
    ["digit1", "Digit 1"],
    ["digit2", "Digit 2"],
    ["digit3", "Digit 3"],
    ["digit4", "Digit 4"],
    ["accent", "Accent"],
    ["seconds", "Seconds"],
    ["timerHours", "Timer Hours"],
    ["timerMinutes", "Timer Minutes"],
    ["timerSeconds", "Timer Seconds"],
    ["stopwatchHours", "Stopwatch Hours"],
    ["stopwatchMinutes", "Stopwatch Minutes"],
    ["stopwatchSeconds", "Stopwatch Seconds"],
  ];

  q("presetEditor").innerHTML = `
    <label><span>Preset Id</span><input id="presetIdInput" value="${preset.id}" /></label>
    <label><span>Name</span><input id="presetNameInput" value="${preset.name}" /></label>
    <label><span>Default Effect</span><select id="presetEffectSelect">${state.metadata.effects
      .map(
        (effect) =>
          `<option value="${effect}" ${effect === preset.effect ? "selected" : ""}>${effect}</option>`,
      )
      .join("")}</select></label>
    <label class="check-row"><span>Rainbow Clock</span><input id="presetRainbowCheckbox" type="checkbox" ${
      preset.rainbowClock ? "checked" : ""
    } /></label>
    <div class="swatch-grid span-2">
      ${colorFields
        .map(
          ([key, label]) => `
            <label>
              <span>${label}</span>
              <input class="preset-color" data-key="${key}" type="color" value="${toHex(
                preset.palette[key],
              )}" />
            </label>`,
        )
        .join("")}
    </div>
    <p class="small-note span-2">Preset changes are local until you press Save Configuration.</p>
  `;

  q("presetIdInput").addEventListener("input", (event) => {
    preset.id = event.target.value;
  });
  q("presetNameInput").addEventListener("input", (event) => {
    preset.name = event.target.value;
  });
  q("presetEffectSelect").addEventListener("change", (event) => {
    preset.effect = event.target.value;
  });
  q("presetRainbowCheckbox").addEventListener("change", (event) => {
    preset.rainbowClock = event.target.checked;
  });
  document.querySelectorAll(".preset-color").forEach((input) => {
    input.addEventListener("input", (event) => {
      preset.palette[event.target.dataset.key] = fromHex(event.target.value);
    });
  });
}

function renderFaceEditor() {
  ensureSelectionBounds();
  const face = state.config.faces[state.selectedFaceIndex];
  fillSelect(
    q("faceSelect"),
    state.config.faces.map((item, index) => String(index)),
    String(state.selectedFaceIndex),
    (value) => state.config.faces[Number(value)]?.name || value,
  );

  const widgetSlots = Array.from({ length: 6 }, (_, index) => face.widgetIds[index] || "");

  q("faceEditor").innerHTML = `
    <label><span>Face Id</span><input id="faceIdInput" value="${face.id}" /></label>
    <label><span>Name</span><input id="faceNameInput" value="${face.name}" /></label>
    <label><span>Base Mode</span><select id="faceBaseModeSelect">${state.metadata.modes
      .map(
        (mode) =>
          `<option value="${mode}" ${mode === face.baseMode ? "selected" : ""}>${mode}</option>`,
      )
      .join("")}</select></label>
    <label><span>Preset</span><select id="facePresetSelect">${state.config.presets
      .map(
        (preset) =>
          `<option value="${preset.id}" ${preset.id === face.presetId ? "selected" : ""}>${preset.name}</option>`,
      )
      .join("")}</select></label>
    <label><span>Effect</span><select id="faceEffectSelect">${state.metadata.effects
      .map(
        (effect) =>
          `<option value="${effect}" ${effect === face.effect ? "selected" : ""}>${effect}</option>`,
      )
      .join("")}</select></label>
    <label class="check-row"><span>Show Seconds</span><input id="faceShowSeconds" type="checkbox" ${
      face.showSeconds ? "checked" : ""
    } /></label>
    <label class="check-row"><span>Show Colon</span><input id="faceShowColon" type="checkbox" ${
      face.showColon ? "checked" : ""
    } /></label>
    <label class="check-row"><span>Show Decimal</span><input id="faceShowDecimal" type="checkbox" ${
      face.showDecimal ? "checked" : ""
    } /></label>
    <label class="check-row"><span>Auto Timer Format</span><input id="faceAutoTimerFormat" type="checkbox" ${
      face.autoTimerFormat ? "checked" : ""
    } /></label>
    ${widgetSlots
      .map(
        (widgetId, index) => `
          <label>
            <span>Widget Slot ${index + 1}</span>
            <select class="face-widget-select" data-index="${index}">
              <option value="">None</option>
              ${state.config.widgets
                .map(
                  (widget) =>
                    `<option value="${widget.id}" ${widget.id === widgetId ? "selected" : ""}>${widget.name}</option>`,
                )
                .join("")}
            </select>
          </label>`,
      )
      .join("")}
  `;

  q("faceIdInput").addEventListener("input", (event) => {
    face.id = event.target.value;
  });
  q("faceNameInput").addEventListener("input", (event) => {
    face.name = event.target.value;
  });
  q("faceBaseModeSelect").addEventListener("change", (event) => {
    face.baseMode = event.target.value;
  });
  q("facePresetSelect").addEventListener("change", (event) => {
    face.presetId = event.target.value;
  });
  q("faceEffectSelect").addEventListener("change", (event) => {
    face.effect = event.target.value;
  });
  q("faceShowSeconds").addEventListener("change", (event) => {
    face.showSeconds = event.target.checked;
  });
  q("faceShowColon").addEventListener("change", (event) => {
    face.showColon = event.target.checked;
  });
  q("faceShowDecimal").addEventListener("change", (event) => {
    face.showDecimal = event.target.checked;
  });
  q("faceAutoTimerFormat").addEventListener("change", (event) => {
    face.autoTimerFormat = event.target.checked;
  });

  document.querySelectorAll(".face-widget-select").forEach((select) => {
    select.addEventListener("change", (event) => {
      const index = Number(event.target.dataset.index);
      const next = [];
      for (let i = 0; i < 6; i += 1) {
        const value = document.querySelector(`.face-widget-select[data-index="${i}"]`).value;
        if (value) next.push(value);
      }
      face.widgetIds = next;
    });
  });
}

function renderWidgetEditor() {
  ensureSelectionBounds();
  const widget = state.config.widgets[state.selectedWidgetIndex];
  fillSelect(
    q("widgetSelect"),
    state.config.widgets.map((item, index) => String(index)),
    String(state.selectedWidgetIndex),
    (value) => state.config.widgets[Number(value)]?.name || value,
  );

  q("widgetEditor").innerHTML = `
    <label><span>Widget Id</span><input id="widgetIdInput" value="${widget.id}" /></label>
    <label><span>Name</span><input id="widgetNameInput" value="${widget.name}" /></label>
    <label><span>Type</span><select id="widgetTypeSelect">${state.metadata.widgetTypes
      .map(
        (type) =>
          `<option value="${type}" ${type === widget.type ? "selected" : ""}>${type}</option>`,
      )
      .join("")}</select></label>
    <label class="check-row"><span>Enabled</span><input id="widgetEnabledInput" type="checkbox" ${
      widget.enabled ? "checked" : ""
    } /></label>
    <label><span>Threshold Seconds</span><input id="widgetThresholdInput" type="number" min="0" max="3600" value="${
      widget.thresholdSeconds
    }" /></label>
    <label><span>Speed</span><input id="widgetSpeedInput" type="number" min="1" max="255" value="${widget.speed}" /></label>
    <label><span>Primary</span><input id="widgetPrimaryInput" type="color" value="${toHex(
      widget.primary,
    )}" /></label>
    <label><span>Secondary</span><input id="widgetSecondaryInput" type="color" value="${toHex(
      widget.secondary,
    )}" /></label>
  `;

  q("widgetIdInput").addEventListener("input", (event) => {
    widget.id = event.target.value;
  });
  q("widgetNameInput").addEventListener("input", (event) => {
    widget.name = event.target.value;
  });
  q("widgetTypeSelect").addEventListener("change", (event) => {
    widget.type = event.target.value;
  });
  q("widgetEnabledInput").addEventListener("change", (event) => {
    widget.enabled = event.target.checked;
  });
  q("widgetThresholdInput").addEventListener("input", (event) => {
    widget.thresholdSeconds = Number(event.target.value || 0);
  });
  q("widgetSpeedInput").addEventListener("input", (event) => {
    widget.speed = Number(event.target.value || 128);
  });
  q("widgetPrimaryInput").addEventListener("input", (event) => {
    widget.primary = fromHex(event.target.value);
  });
  q("widgetSecondaryInput").addEventListener("input", (event) => {
    widget.secondary = fromHex(event.target.value);
  });
}

function renderMapping() {
  const body = q("mappingTableBody");
  body.innerHTML = "";
  const targets = [{ id: 255, name: "UNUSED" }, ...state.metadata.logicalTargets];
  state.config.mapping.forEach((logical, physical) => {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${physical}</td>
      <td>
        <select data-physical="${physical}">
          ${targets
            .map(
              (target) =>
                `<option value="${target.id}" ${
                  Number(target.id) === Number(logical) ? "selected" : ""
                }>${target.name}</option>`,
            )
            .join("")}
        </select>
      </td>
    `;
    body.appendChild(row);
  });

  body.querySelectorAll("select").forEach((select) => {
    select.addEventListener("change", (event) => {
      state.config.mapping[Number(event.target.dataset.physical)] = Number(event.target.value);
      renderMappingSummary();
    });
  });

  fillSelect(
    q("assignLogicalSelect"),
    targets.map((item) => String(item.id)),
    "255",
    (value) => targets.find((item) => String(item.id) === value)?.name || value,
  );
  renderMappingSummary();
}

function renderMappingSummary() {
  const used = new Map();
  state.config.mapping.forEach((logical, physical) => {
    if (logical === 255) return;
    if (!used.has(logical)) used.set(logical, []);
    used.get(logical).push(physical);
  });
  const duplicates = [...used.entries()].filter(([, physical]) => physical.length > 1);
  const assigned = state.config.mapping.filter((logical) => logical !== 255).length;
  q("mappingSummary").textContent = `Assigned ${assigned}/31 LEDs. Duplicate groups: ${duplicates.length}. Calibration cursor: ${state.status?.calibrationCursor ?? 0}.`;
}

function renderNetworkEditor() {
  const network = state.config.network;
  q("networkEditor").innerHTML = `
    <label><span>Access Point SSID</span><input id="networkApSsid" value="${network.apSsid}" /></label>
    <label><span>Access Point Password</span><input id="networkApPassword" value="${network.apPassword}" /></label>
    <label class="check-row"><span>Enable STA Join</span><input id="networkStaEnabled" type="checkbox" ${
      network.staEnabled ? "checked" : ""
    } /></label>
    <label><span>Station SSID</span><input id="networkStaSsid" value="${network.staSsid}" /></label>
    <label><span>Station Password</span><input id="networkStaPassword" value="${network.staPassword}" /></label>
    <label><span>Hostname</span><input id="networkHostname" value="${network.hostname}" /></label>
    <p class="small-note">Network and hardware pin changes may require reboot to fully apply.</p>
  `;

  q("networkApSsid").addEventListener("input", (event) => {
    network.apSsid = event.target.value;
  });
  q("networkApPassword").addEventListener("input", (event) => {
    network.apPassword = event.target.value;
  });
  q("networkStaEnabled").addEventListener("change", (event) => {
    network.staEnabled = event.target.checked;
  });
  q("networkStaSsid").addEventListener("input", (event) => {
    network.staSsid = event.target.value;
  });
  q("networkStaPassword").addEventListener("input", (event) => {
    network.staPassword = event.target.value;
  });
  q("networkHostname").addEventListener("input", (event) => {
    network.hostname = event.target.value;
  });
}

function renderEverything() {
  renderStatusRibbon();
  renderRuntimeControls();
  renderPresetEditor();
  renderFaceEditor();
  renderWidgetEditor();
  renderMapping();
  renderNetworkEditor();
}

function bindStaticControls() {
  q("modeSelect").addEventListener("change", (event) => {
    state.config.runtime.mode = event.target.value;
  });
  q("hourModeSelect").addEventListener("change", (event) => {
    state.config.runtime.hourMode = event.target.value;
  });
  q("secondsModeSelect").addEventListener("change", (event) => {
    state.config.runtime.secondsMode = event.target.value;
  });
  q("timerFormatSelect").addEventListener("change", (event) => {
    state.config.runtime.timerFormat = event.target.value;
  });
  q("effectSelect").addEventListener("change", (event) => {
    state.config.runtime.effect = event.target.value;
  });
  q("activePresetSelect").addEventListener("change", (event) => {
    state.config.runtime.activePresetId = event.target.value;
  });
  q("activeFaceSelect").addEventListener("change", (event) => {
    state.config.runtime.activeFaceId = event.target.value;
  });
  q("brightnessRange").addEventListener("input", (event) => {
    state.config.runtime.brightness = Number(event.target.value);
    q("brightnessValue").textContent = event.target.value;
  });
  q("colonBrightnessRange").addEventListener("input", (event) => {
    state.config.runtime.colonBrightness = Number(event.target.value);
    q("colonBrightnessValue").textContent = event.target.value;
  });
  q("rainbowEnabledCheckbox").addEventListener("change", (event) => {
    state.config.runtime.rainbowEnabled = event.target.checked;
  });

  q("presetSelect").addEventListener("change", (event) => {
    state.selectedPresetIndex = Number(event.target.value);
    renderPresetEditor();
  });
  q("faceSelect").addEventListener("change", (event) => {
    state.selectedFaceIndex = Number(event.target.value);
    renderFaceEditor();
  });
  q("widgetSelect").addEventListener("change", (event) => {
    state.selectedWidgetIndex = Number(event.target.value);
    renderWidgetEditor();
  });

  q("newPresetButton").addEventListener("click", () => {
    if (state.config.presets.length >= 12) {
      log("Preset limit reached");
      return;
    }
    state.config.presets.push(clone(state.config.presets[state.selectedPresetIndex]));
    state.selectedPresetIndex = state.config.presets.length - 1;
    state.config.presets[state.selectedPresetIndex].id = `preset-${Date.now()}`;
    state.config.presets[state.selectedPresetIndex].name = "New Preset";
    renderEverything();
  });
  q("deletePresetButton").addEventListener("click", () => {
    if (state.config.presets.length <= 1) return;
    state.config.presets.splice(state.selectedPresetIndex, 1);
    ensureSelectionBounds();
    renderEverything();
  });
  q("applyPresetButton").addEventListener("click", async () => {
    const presetId = state.config.presets[state.selectedPresetIndex].id;
    state.config.runtime.activePresetId = presetId;
    renderRuntimeControls();
    await runAction({ action: "apply_preset", presetId }, `Applied preset ${presetId}`);
  });

  q("newFaceButton").addEventListener("click", () => {
    if (state.config.faces.length >= 12) {
      log("Face limit reached");
      return;
    }
    state.config.faces.push(clone(state.config.faces[state.selectedFaceIndex]));
    state.selectedFaceIndex = state.config.faces.length - 1;
    state.config.faces[state.selectedFaceIndex].id = `face-${Date.now()}`;
    state.config.faces[state.selectedFaceIndex].name = "New Face";
    renderEverything();
  });
  q("deleteFaceButton").addEventListener("click", () => {
    if (state.config.faces.length <= 1) return;
    state.config.faces.splice(state.selectedFaceIndex, 1);
    ensureSelectionBounds();
    renderEverything();
  });
  q("applyFaceButton").addEventListener("click", async () => {
    const faceId = state.config.faces[state.selectedFaceIndex].id;
    state.config.runtime.activeFaceId = faceId;
    renderRuntimeControls();
    await runAction({ action: "apply_face", faceId }, `Activated face ${faceId}`);
  });

  q("newWidgetButton").addEventListener("click", () => {
    if (state.config.widgets.length >= 12) {
      log("Widget limit reached");
      return;
    }
    state.config.widgets.push(clone(state.config.widgets[state.selectedWidgetIndex]));
    state.selectedWidgetIndex = state.config.widgets.length - 1;
    state.config.widgets[state.selectedWidgetIndex].id = `widget-${Date.now()}`;
    state.config.widgets[state.selectedWidgetIndex].name = "New Widget";
    renderEverything();
  });
  q("deleteWidgetButton").addEventListener("click", () => {
    if (state.config.widgets.length <= 1) return;
    state.config.widgets.splice(state.selectedWidgetIndex, 1);
    ensureSelectionBounds();
    renderEverything();
  });

  q("timerSetButton").addEventListener("click", async () => {
    const seconds =
      Number(q("timerHoursInput").value || 0) * 3600 +
      Number(q("timerMinutesInput").value || 0) * 60 +
      Number(q("timerSecondsInput").value || 0);
    state.config.runtime.timerPresetSeconds = seconds;
    await runAction({ action: "timer_set", seconds }, `Timer set to ${seconds}s`);
  });
  q("timerStartButton").addEventListener("click", () => runAction({ action: "timer_start" }, "Timer started"));
  q("timerStopButton").addEventListener("click", () => runAction({ action: "timer_stop" }, "Timer stopped"));
  q("timerResetButton").addEventListener("click", () => runAction({ action: "timer_reset" }, "Timer reset"));
  q("stopwatchStartButton").addEventListener("click", () => runAction({ action: "stopwatch_start" }, "Stopwatch started"));
  q("stopwatchStopButton").addEventListener("click", () => runAction({ action: "stopwatch_stop" }, "Stopwatch stopped"));
  q("stopwatchResetButton").addEventListener("click", () => runAction({ action: "stopwatch_reset" }, "Stopwatch reset"));

  q("rtcSetButton").addEventListener("click", async () => {
    const value = q("rtcDateTimeInput").value;
    if (!value) return;
    const date = new Date(value);
    await runAction(
      {
        action: "rtc_set",
        year: date.getFullYear(),
        month: date.getMonth() + 1,
        day: date.getDate(),
        hour: date.getHours(),
        minute: date.getMinutes(),
        second: date.getSeconds(),
      },
      "RTC updated",
    );
  });

  q("calibrationEnableButton").addEventListener("click", () =>
    runAction({ action: "calibration_mode", enabled: true }, "Calibration mode enabled"),
  );
  q("calibrationDisableButton").addEventListener("click", () =>
    runAction({ action: "calibration_mode", enabled: false }, "Calibration mode disabled"),
  );
  q("calibrationPrevButton").addEventListener("click", () =>
    runAction({ action: "calibration_prev" }, "Moved to previous LED"),
  );
  q("calibrationNextButton").addEventListener("click", () =>
    runAction({ action: "calibration_next" }, "Moved to next LED"),
  );
  q("assignLogicalButton").addEventListener("click", () =>
    runAction(
      { action: "assign", logical: Number(q("assignLogicalSelect").value) },
      "Assigned current calibration target",
    ),
  );
  q("unassignLogicalButton").addEventListener("click", () =>
    runAction({ action: "unassign" }, "Unassigned current calibration target"),
  );
  q("testSegmentsButton").addEventListener("click", () =>
    runAction({ action: "test_segments" }, "Running segment test"),
  );
  q("testDigitsButton").addEventListener("click", () =>
    runAction({ action: "test_digits" }, "Running digit test"),
  );
  q("testAllButton").addEventListener("click", () =>
    runAction({ action: "test_all" }, "Running full test"),
  );

  q("saveConfigButton").addEventListener("click", saveConfig);
  q("loadConfigButton").addEventListener("click", () => refreshBootstrap(true, true));
  q("refreshBootstrapButton").addEventListener("click", () => refreshBootstrap(true, true));
}

async function runAction(payload, successMessage) {
  try {
    const response = await postJson("/api/action", payload);
    state.status = response.status;
    state.rtc = response.rtc;
    renderStatusRibbon();
    log(successMessage || response.message || payload.action);
  } catch (error) {
    log(error.message);
  }
}

async function saveConfig() {
  try {
    const response = await postJson("/api/config", state.config);
    state.status = response.status;
    state.rtc = response.rtc;
    log(response.restartRequired ? "Configuration saved. Restart recommended." : "Configuration saved.");
    await refreshBootstrap(false, true);
  } catch (error) {
    log(error.message);
  }
}

async function refreshBootstrap(logRefresh = true, forceReplace = false) {
  try {
    const data = await getJson("/api/bootstrap");
    state.info = data.info;
    state.status = data.status;
    state.rtc = data.rtc;
    state.metadata = data.metadata;
    state.network = data.network;
    if (!state.config || logRefresh || forceReplace) {
      state.config = clone(data.config);
    }
    renderEverything();
    if (logRefresh) log("Bootstrap refreshed");
  } catch (error) {
    log(error.message);
  }
}

async function boot() {
  bindStaticControls();
  await refreshBootstrap(false, true);
  renderEverything();
  log("Clock Studio ready");
  window.setInterval(async () => {
    try {
      const data = await getJson("/api/bootstrap");
      state.info = data.info;
      state.status = data.status;
      state.rtc = data.rtc;
      state.network = data.network;
      renderStatusRibbon();
      renderMappingSummary();
    } catch (error) {
      log(error.message);
    }
  }, 2000);
}

window.addEventListener("DOMContentLoaded", boot);
