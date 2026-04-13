from __future__ import annotations

import json
import sys
from dataclasses import dataclass, field
from datetime import datetime
from enum import IntEnum
from pathlib import Path
from typing import List, Optional, Set

import serial
from PySide6 import QtCore, QtGui, QtWidgets


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


LOG_UNUSED = 255


@dataclass
class MappingModel:
    num_pixels: int = 31
    pixel_pin: int = 5
    phys_to_logical: List[int] = field(default_factory=lambda: [LOG_UNUSED] * 31)

    def assign(self, phys: int, logical: int) -> None:
        if not (0 <= phys < self.num_pixels):
            raise ValueError(f"phys index out of range: {phys}")
        self.phys_to_logical[phys] = logical

    def used_logicals(self) -> Set[int]:
        return {v for v in self.phys_to_logical if v != LOG_UNUSED}

    def validate(self) -> List[str]:
        errors: List[str] = []
        used = [v for v in self.phys_to_logical if v != LOG_UNUSED]
        if len(used) != len(set(used)):
            errors.append("Duplicate logical assignment detected.")
        expected = set(int(x) for x in LogicalId)
        missing = sorted(expected - self.used_logicals())
        if missing:
            errors.append(f"Unassigned logical positions: {len(missing)}")
        return errors

    def to_json_obj(self) -> dict:
        names = []
        for v in self.phys_to_logical:
            names.append("UNUSED" if v == LOG_UNUSED else LogicalId(v).name)
        return {
            "schema": "neopixel-clock-map/v1",
            "num_pixels": self.num_pixels,
            "pixel_pin": self.pixel_pin,
            "color_order": "GRB",
            "phys_to_logical": names,
            "notes": {"digit_order": "D1 D2 : D3 D4"},
        }

    def export_header(self) -> str:
        logical_count = len(list(LogicalId))
        logical_to_phys = [255] * logical_count
        for phys, logical in enumerate(self.phys_to_logical):
            if logical != LOG_UNUSED and logical < logical_count:
                logical_to_phys[logical] = phys

        def arr(vals: List[int]) -> str:
            return "{ " + ", ".join(str(v) for v in vals) + " }"

        return "\n".join(
            [
                "#pragma once",
                "#include <stdint.h>",
                '#include "LogicalIds.h"',
                "",
                f"constexpr uint8_t kPhysToLogical[kNumPixels] = {arr(self.phys_to_logical)};",
                f"constexpr uint8_t kLogicalToPhys[LOGICAL_COUNT] = {arr(logical_to_phys)};",
                "",
            ]
        )


class SerialWorker(QtCore.QObject):
    line_rx = QtCore.Signal(str)
    connected = QtCore.Signal(bool, str)

    def __init__(self) -> None:
        super().__init__()
        self._ser: Optional[serial.Serial] = None
        self._timer = QtCore.QTimer()
        self._timer.setInterval(10)
        self._timer.timeout.connect(self._poll)

    @QtCore.Slot(str)
    def connect_port(self, port: str) -> None:
        try:
            self._ser = serial.Serial(port=port, baudrate=115200, timeout=0)
            self._timer.start()
            self.connected.emit(True, f"Connected: {port}")
        except Exception as e:
            self.connected.emit(False, f"Connect failed: {e}")

    def send(self, line: str) -> None:
        if not self._ser:
            return
        self._ser.write((line.strip() + "\n").encode("utf-8"))

    def _poll(self) -> None:
        if not self._ser:
            return
        try:
            while self._ser.in_waiting:
                raw = self._ser.readline()
                if not raw:
                    break
                self.line_rx.emit(raw.decode("utf-8", errors="replace").strip())
        except Exception as e:
            self.connected.emit(False, f"Serial error: {e}")
            self._timer.stop()
            self._ser = None


class ColorButton(QtWidgets.QPushButton):
    color_changed = QtCore.Signal(QtGui.QColor)

    def __init__(self, title: str, initial: QtGui.QColor) -> None:
        super().__init__(title)
        self._color = initial
        self.clicked.connect(self._pick_color)
        self._refresh_style()

    def color(self) -> QtGui.QColor:
        return self._color

    def set_color(self, color: QtGui.QColor) -> None:
        self._color = color
        self._refresh_style()
        self.color_changed.emit(color)

    def rgb_tuple(self) -> tuple[int, int, int]:
        return self._color.red(), self._color.green(), self._color.blue()

    def _pick_color(self) -> None:
        color = QtWidgets.QColorDialog.getColor(self._color, self.window(), "Pick Color")
        if color.isValid():
            self.set_color(color)

    def _refresh_style(self) -> None:
        r, g, b = self.rgb_tuple()
        brightness = (r * 299 + g * 587 + b * 114) / 1000
        text = "black" if brightness > 140 else "white"
        self.setText(f"{self.text().split(':')[0]}: {r}, {g}, {b}")
        self.setStyleSheet(
            f"background-color: rgb({r}, {g}, {b}); color: {text}; font-weight: 600; padding: 6px;"
        )


class MainWindow(QtWidgets.QWidget):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle("ClockCal + Clock Control")
        self.model = MappingModel()
        self.cur_phys = 0

        self.port_box = QtWidgets.QLineEdit()
        self.port_box.setPlaceholderText("COM3 or /dev/ttyACM0")
        self.connect_btn = QtWidgets.QPushButton("Connect")

        self.hello_btn = QtWidgets.QPushButton("HELLO")
        self.info_btn = QtWidgets.QPushButton("INFO?")
        self.prev_btn = QtWidgets.QPushButton("Prev")
        self.next_btn = QtWidgets.QPushButton("Next")
        self.map_btn = QtWidgets.QPushButton("MAP?")
        self.validate_btn = QtWidgets.QPushButton("VALIDATE?")
        self.test_seg_btn = QtWidgets.QPushButton("TEST SEGMENTS")
        self.test_digits_btn = QtWidgets.QPushButton("TEST DIGITS")
        self.save_dev_btn = QtWidgets.QPushButton("SAVE to Device")
        self.load_dev_btn = QtWidgets.QPushButton("LOAD from Device")

        self.logical_box = QtWidgets.QComboBox()
        self.logical_box.addItem("UNUSED", LOG_UNUSED)
        for lid in LogicalId:
            self.logical_box.addItem(lid.name, int(lid))
        self.assign_btn = QtWidgets.QPushButton("Assign + Next")
        self.save_json_btn = QtWidgets.QPushButton("Save JSON")
        self.save_h_btn = QtWidgets.QPushButton("Export .h")

        self.mode_box = QtWidgets.QComboBox()
        self.mode_box.addItems(["CLOCK", "CLOCK_SECONDS", "TIMER", "STOPWATCH", "COLOR_DEMO"])
        self.set_mode_btn = QtWidgets.QPushButton("Set Mode")

        self.hour_mode_box = QtWidgets.QComboBox()
        self.hour_mode_box.addItems(["24H", "12H"])
        self.set_hour_mode_btn = QtWidgets.QPushButton("Set Hour Mode")

        self.seconds_mode_box = QtWidgets.QComboBox()
        self.seconds_mode_box.addItems(["BLINK", "ON", "OFF"])
        self.set_seconds_mode_btn = QtWidgets.QPushButton("Set Seconds Mode")

        self.colon_brightness_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.colon_brightness_slider.setRange(0, 255)
        self.colon_brightness_slider.setValue(255)
        self.colon_brightness_value = QtWidgets.QLabel("255")
        self.set_colon_brightness_btn = QtWidgets.QPushButton("Send Colon Brightness")

        self.brightness_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.brightness_slider.setRange(1, 255)
        self.brightness_slider.setValue(64)
        self.brightness_value = QtWidgets.QLabel("64")
        self.set_brightness_btn = QtWidgets.QPushButton("Send Brightness")

        self.clock_color_btn = ColorButton("Clock Color", QtGui.QColor(255, 120, 40))
        self.accent_color_btn = ColorButton("Accent Color", QtGui.QColor(0, 120, 255))
        self.seconds_color_btn = ColorButton("Seconds Color", QtGui.QColor(255, 255, 255))
        self.timer_color_btn = ColorButton("Timer Color", QtGui.QColor(80, 255, 120))
        self.send_colors_btn = QtWidgets.QPushButton("Send All Colors")
        self.send_accent_color_btn = QtWidgets.QPushButton("Send Colon Color")

        self.hours_color_btn = ColorButton("Hours Color", QtGui.QColor(255, 120, 40))
        self.minutes_color_btn = ColorButton("Minutes Color", QtGui.QColor(255, 120, 40))
        self.digit1_color_btn = ColorButton("Digit 1 Color", QtGui.QColor(255, 120, 40))
        self.digit2_color_btn = ColorButton("Digit 2 Color", QtGui.QColor(255, 120, 40))
        self.digit3_color_btn = ColorButton("Digit 3 Color", QtGui.QColor(255, 120, 40))
        self.digit4_color_btn = ColorButton("Digit 4 Color", QtGui.QColor(255, 120, 40))

        self.send_hours_color_btn = QtWidgets.QPushButton("Send Hours Color")
        self.send_minutes_color_btn = QtWidgets.QPushButton("Send Minutes Color")
        self.send_digit_colors_btn = QtWidgets.QPushButton("Send Individual Digit Colors")

        self.color_preset_box = QtWidgets.QComboBox()
        self.color_preset_box.addItems([
            "Warm Classic",
            "Ice Blue",
            "Matrix",
            "Sunset",
            "Rainbow",
            "Party",
        ])
        self.apply_preset_btn = QtWidgets.QPushButton("Apply Preset")

        self.read_rtc_btn = QtWidgets.QPushButton("Read RTC")
        self.set_rtc_now_btn = QtWidgets.QPushButton("Set RTC From PC Time")
        self.datetime_edit = QtWidgets.QDateTimeEdit(QtCore.QDateTime.currentDateTime())
        self.datetime_edit.setCalendarPopup(True)
        self.set_rtc_manual_btn = QtWidgets.QPushButton("Set RTC Manual")

        self.timer_minutes = QtWidgets.QSpinBox()
        self.timer_minutes.setRange(0, 999)
        self.timer_minutes.setValue(5)
        self.timer_seconds = QtWidgets.QSpinBox()
        self.timer_seconds.setRange(0, 59)
        self.timer_seconds.setValue(0)
        self.timer_set_btn = QtWidgets.QPushButton("Set Timer")
        self.timer_start_btn = QtWidgets.QPushButton("Start Timer")
        self.timer_stop_btn = QtWidgets.QPushButton("Stop Timer")
        self.timer_reset_btn = QtWidgets.QPushButton("Reset Timer")

        self.sw_start_btn = QtWidgets.QPushButton("Start Stopwatch")
        self.sw_stop_btn = QtWidgets.QPushButton("Stop Stopwatch")
        self.sw_reset_btn = QtWidgets.QPushButton("Reset Stopwatch")

        self.custom_command = QtWidgets.QLineEdit()
        self.custom_command.setPlaceholderText("Send raw serial command to the firmware")
        self.custom_send_btn = QtWidgets.QPushButton("Send Raw Command")

        self.status_label = QtWidgets.QLabel("Disconnected")
        self.current_phys_label = QtWidgets.QLabel("Current physical LED: 0")
        self.rtc_label = QtWidgets.QLabel("RTC: unknown")
        self.log = QtWidgets.QPlainTextEdit()
        self.log.setReadOnly(True)
        self.log.setMaximumBlockCount(800)
        self.log.setMinimumHeight(170)

        self._group_title_style = "QGroupBox { font-weight: 600; }"
        self.setStyleSheet(
            self._group_title_style
            + "QPushButton { padding: 4px 8px; }"
            + "QLabel { min-width: 0px; }"
            + "QComboBox { min-width: 110px; }"
        )

        self._build_layout()

        self.worker = SerialWorker()
        self.thread = QtCore.QThread()
        self.worker.moveToThread(self.thread)
        self.thread.start()

        self.connect_btn.clicked.connect(self._connect)
        self.hello_btn.clicked.connect(lambda: self._send("HELLO ClockCal/1"))
        self.info_btn.clicked.connect(lambda: self._send("INFO?"))
        self.prev_btn.clicked.connect(lambda: self._send("PREV"))
        self.next_btn.clicked.connect(lambda: self._send("NEXT"))
        self.map_btn.clicked.connect(lambda: self._send("MAP?"))
        self.validate_btn.clicked.connect(lambda: self._send("VALIDATE?"))
        self.test_seg_btn.clicked.connect(lambda: self._send("TEST SEGMENTS"))
        self.test_digits_btn.clicked.connect(lambda: self._send("TEST DIGITS"))
        self.save_dev_btn.clicked.connect(lambda: self._send("SAVE"))
        self.load_dev_btn.clicked.connect(lambda: self._send("LOAD"))
        self.assign_btn.clicked.connect(self._assign_next)
        self.save_json_btn.clicked.connect(self._save_json)
        self.save_h_btn.clicked.connect(self._save_h)

        self.set_mode_btn.clicked.connect(self._send_mode)
        self.set_hour_mode_btn.clicked.connect(self._send_hour_mode)
        self.set_seconds_mode_btn.clicked.connect(self._send_seconds_mode)
        self.colon_brightness_slider.valueChanged.connect(
            lambda v: self.colon_brightness_value.setText(str(v))
        )
        self.set_colon_brightness_btn.clicked.connect(self._send_colon_brightness)
        self.brightness_slider.valueChanged.connect(lambda v: self.brightness_value.setText(str(v)))
        self.set_brightness_btn.clicked.connect(self._send_brightness)
        self.send_colors_btn.clicked.connect(self._send_all_colors)
        self.send_accent_color_btn.clicked.connect(self._send_accent_color)
        self.send_hours_color_btn.clicked.connect(self._send_hours_color)
        self.send_minutes_color_btn.clicked.connect(self._send_minutes_color)
        self.send_digit_colors_btn.clicked.connect(self._send_digit_colors)
        self.apply_preset_btn.clicked.connect(self._apply_color_preset)

        self.read_rtc_btn.clicked.connect(lambda: self._send("RTC READ"))
        self.set_rtc_now_btn.clicked.connect(self._set_rtc_from_pc)
        self.set_rtc_manual_btn.clicked.connect(self._set_rtc_manual)

        self.timer_set_btn.clicked.connect(self._set_timer)
        self.timer_start_btn.clicked.connect(lambda: self._send("TIMER START"))
        self.timer_stop_btn.clicked.connect(lambda: self._send("TIMER STOP"))
        self.timer_reset_btn.clicked.connect(lambda: self._send("TIMER RESET"))

        self.sw_start_btn.clicked.connect(lambda: self._send("STOPWATCH START"))
        self.sw_stop_btn.clicked.connect(lambda: self._send("STOPWATCH STOP"))
        self.sw_reset_btn.clicked.connect(lambda: self._send("STOPWATCH RESET"))

        self.custom_send_btn.clicked.connect(self._send_custom_command)
        self.custom_command.returnPressed.connect(self._send_custom_command)

        self.worker.connected.connect(self._on_conn)
        self.worker.line_rx.connect(self._on_line)

    def _build_layout(self) -> None:
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setSpacing(8)

        top = QtWidgets.QHBoxLayout()
        top.setSpacing(6)
        top.addWidget(self.port_box)
        top.addWidget(self.connect_btn)
        layout.addLayout(top)

        layout.addWidget(self.status_label)
        layout.addWidget(self.current_phys_label)
        layout.addWidget(self.rtc_label)

        main_row = QtWidgets.QHBoxLayout()
        main_row.setSpacing(8)
        layout.addLayout(main_row, 1)

        controls_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        controls_splitter.setChildrenCollapsible(False)
        main_row.addWidget(controls_splitter, 3)

        left_panel = QtWidgets.QWidget()
        right_panel = QtWidgets.QWidget()
        left_col = QtWidgets.QVBoxLayout(left_panel)
        right_col = QtWidgets.QVBoxLayout(right_panel)
        left_col.setContentsMargins(0, 0, 0, 0)
        right_col.setContentsMargins(0, 0, 0, 0)
        left_col.setSpacing(8)
        right_col.setSpacing(8)
        controls_splitter.addWidget(left_panel)
        controls_splitter.addWidget(right_panel)
        controls_splitter.setSizes([640, 420])

        cal_group = QtWidgets.QGroupBox("Calibration")
        cal_layout = QtWidgets.QVBoxLayout(cal_group)
        cal_layout.setSpacing(6)

        row1 = QtWidgets.QGridLayout()
        row1.setHorizontalSpacing(6)
        row1.setVerticalSpacing(6)
        row1.addWidget(self.hello_btn, 0, 0)
        row1.addWidget(self.info_btn, 0, 1)
        row1.addWidget(self.map_btn, 1, 0)
        row1.addWidget(self.validate_btn, 1, 1)
        cal_layout.addLayout(row1)

        row2 = QtWidgets.QGridLayout()
        row2.setHorizontalSpacing(6)
        row2.setVerticalSpacing(6)
        row2.addWidget(self.prev_btn, 0, 0)
        row2.addWidget(self.next_btn, 0, 1)
        row2.addWidget(self.test_seg_btn, 1, 0)
        row2.addWidget(self.test_digits_btn, 1, 1)
        cal_layout.addLayout(row2)

        row3 = QtWidgets.QHBoxLayout()
        row3.addWidget(self.logical_box)
        row3.addWidget(self.assign_btn)
        cal_layout.addLayout(row3)

        row4 = QtWidgets.QGridLayout()
        row4.setHorizontalSpacing(6)
        row4.setVerticalSpacing(6)
        row4.addWidget(self.save_dev_btn, 0, 0)
        row4.addWidget(self.load_dev_btn, 0, 1)
        row4.addWidget(self.save_json_btn, 1, 0)
        row4.addWidget(self.save_h_btn, 1, 1)
        cal_layout.addLayout(row4)

        clock_group = QtWidgets.QGroupBox("Clock Controls")
        clock_layout = QtWidgets.QVBoxLayout(clock_group)
        clock_layout.setSpacing(6)

        mode_row = QtWidgets.QHBoxLayout()
        mode_row.addWidget(QtWidgets.QLabel("Mode"))
        mode_row.addWidget(self.mode_box)
        mode_row.addWidget(self.set_mode_btn)
        clock_layout.addLayout(mode_row)

        hour_row = QtWidgets.QHBoxLayout()
        hour_row.addWidget(QtWidgets.QLabel("Hour Format"))
        hour_row.addWidget(self.hour_mode_box)
        hour_row.addWidget(self.set_hour_mode_btn)
        clock_layout.addLayout(hour_row)

        seconds_row = QtWidgets.QHBoxLayout()
        seconds_row.addWidget(QtWidgets.QLabel("Colon Mode"))
        seconds_row.addWidget(self.seconds_mode_box)
        seconds_row.addWidget(self.set_seconds_mode_btn)
        clock_layout.addLayout(seconds_row)

        colon_row = QtWidgets.QHBoxLayout()
        colon_row.addWidget(QtWidgets.QLabel(": Brightness"))
        colon_row.addWidget(self.colon_brightness_slider)
        colon_row.addWidget(self.colon_brightness_value)
        colon_row.addWidget(self.set_colon_brightness_btn)
        clock_layout.addLayout(colon_row)

        bright_row = QtWidgets.QHBoxLayout()
        bright_row.addWidget(QtWidgets.QLabel("Brightness"))
        bright_row.addWidget(self.brightness_slider)
        bright_row.addWidget(self.brightness_value)
        bright_row.addWidget(self.set_brightness_btn)
        clock_layout.addLayout(bright_row)

        base_colors_group = QtWidgets.QGroupBox("Base Colors")
        base_colors_layout = QtWidgets.QVBoxLayout(base_colors_group)
        base_colors_layout.setSpacing(6)
        color_grid = QtWidgets.QGridLayout()
        color_grid.setHorizontalSpacing(6)
        color_grid.setVerticalSpacing(6)
        color_grid.addWidget(self.clock_color_btn, 0, 0)
        color_grid.addWidget(self.accent_color_btn, 0, 1)
        color_grid.addWidget(self.seconds_color_btn, 1, 0)
        color_grid.addWidget(self.timer_color_btn, 1, 1)
        base_colors_layout.addLayout(color_grid)
        color_button_row = QtWidgets.QHBoxLayout()
        color_button_row.addWidget(self.send_colors_btn)
        color_button_row.addWidget(self.send_accent_color_btn)
        base_colors_layout.addLayout(color_button_row)
        clock_layout.addWidget(base_colors_group)

        hm_colors_group = QtWidgets.QGroupBox("Hour / Minute Colors")
        hm_colors_layout = QtWidgets.QGridLayout(hm_colors_group)
        hm_colors_layout.setHorizontalSpacing(6)
        hm_colors_layout.setVerticalSpacing(6)
        hm_colors_layout.addWidget(self.hours_color_btn, 0, 0)
        hm_colors_layout.addWidget(self.minutes_color_btn, 0, 1)
        hm_colors_layout.addWidget(self.send_hours_color_btn, 1, 0)
        hm_colors_layout.addWidget(self.send_minutes_color_btn, 1, 1)
        clock_layout.addWidget(hm_colors_group)

        digit_colors_group = QtWidgets.QGroupBox("Individual Digit Colors")
        digit_colors_layout = QtWidgets.QVBoxLayout(digit_colors_group)
        digit_colors_layout.setSpacing(6)
        digit_grid = QtWidgets.QGridLayout()
        digit_grid.setHorizontalSpacing(6)
        digit_grid.setVerticalSpacing(6)
        digit_grid.addWidget(self.digit1_color_btn, 0, 0)
        digit_grid.addWidget(self.digit2_color_btn, 0, 1)
        digit_grid.addWidget(self.digit3_color_btn, 1, 0)
        digit_grid.addWidget(self.digit4_color_btn, 1, 1)
        digit_colors_layout.addLayout(digit_grid)
        digit_colors_layout.addWidget(self.send_digit_colors_btn)
        clock_layout.addWidget(digit_colors_group)

        preset_row = QtWidgets.QHBoxLayout()
        preset_row.addWidget(QtWidgets.QLabel("Color Preset"))
        preset_row.addWidget(self.color_preset_box, 1)
        preset_row.addWidget(self.apply_preset_btn)
        clock_layout.addLayout(preset_row)

        rtc_group = QtWidgets.QGroupBox("RTC")
        rtc_layout = QtWidgets.QVBoxLayout(rtc_group)
        rtc_layout.setSpacing(6)
        rtc_row1 = QtWidgets.QHBoxLayout()
        rtc_row1.addWidget(self.read_rtc_btn)
        rtc_row1.addWidget(self.set_rtc_now_btn)
        rtc_layout.addLayout(rtc_row1)
        rtc_row2 = QtWidgets.QHBoxLayout()
        rtc_row2.addWidget(self.datetime_edit)
        rtc_row2.addWidget(self.set_rtc_manual_btn)
        rtc_layout.addLayout(rtc_row2)

        timer_group = QtWidgets.QGroupBox("Timer")
        timer_layout = QtWidgets.QVBoxLayout(timer_group)
        timer_layout.setSpacing(6)
        timer_row1 = QtWidgets.QHBoxLayout()
        timer_row1.addWidget(QtWidgets.QLabel("Minutes"))
        timer_row1.addWidget(self.timer_minutes)
        timer_row1.addWidget(QtWidgets.QLabel("Seconds"))
        timer_row1.addWidget(self.timer_seconds)
        timer_row1.addWidget(self.timer_set_btn)
        timer_layout.addLayout(timer_row1)
        timer_row2 = QtWidgets.QHBoxLayout()
        timer_row2.addWidget(self.timer_start_btn)
        timer_row2.addWidget(self.timer_stop_btn)
        timer_row2.addWidget(self.timer_reset_btn)
        timer_layout.addLayout(timer_row2)

        sw_group = QtWidgets.QGroupBox("Stopwatch")
        sw_layout = QtWidgets.QHBoxLayout(sw_group)
        sw_layout.addWidget(self.sw_start_btn)
        sw_layout.addWidget(self.sw_stop_btn)
        sw_layout.addWidget(self.sw_reset_btn)

        raw_group = QtWidgets.QGroupBox("Raw Command")
        raw_layout = QtWidgets.QHBoxLayout(raw_group)
        raw_layout.addWidget(self.custom_command)
        raw_layout.addWidget(self.custom_send_btn)

        left_col.addWidget(cal_group)
        left_col.addWidget(clock_group, 1)
        right_col.addWidget(rtc_group)
        right_col.addWidget(timer_group)
        right_col.addWidget(sw_group)
        right_col.addWidget(raw_group)
        right_col.addStretch(1)

        log_group = QtWidgets.QGroupBox("Log")
        log_layout = QtWidgets.QVBoxLayout(log_group)
        log_layout.setContentsMargins(6, 6, 6, 6)
        log_layout.addWidget(self.log)
        log_group.setMinimumWidth(320)
        log_group.setMaximumWidth(420)
        main_row.addWidget(log_group, 2)

    def _connect(self) -> None:
        port = self.port_box.text().strip()
        QtCore.QMetaObject.invokeMethod(
            self.worker,
            "connect_port",
            QtCore.Qt.QueuedConnection,
            QtCore.Q_ARG(str, port),
        )

    def _send(self, command: str) -> None:
        self.log.appendPlainText(f">> {command}")
        self.worker.send(command)

    def _on_conn(self, ok: bool, msg: str) -> None:
        self.status_label.setText(msg)
        self.log.appendPlainText(msg)
        if ok:
            self._send("HELLO ClockCal/1")
            self._send("RTC READ")

    def _on_line(self, line: str) -> None:
        self.log.appendPlainText("<< " + line)

        if line.startswith("CUR "):
            self.cur_phys = int(line.split()[1])
            self.current_phys_label.setText(f"Current physical LED: {self.cur_phys}")

        if line.startswith("MAP "):
            parts = [int(x) for x in line[4:].strip().split(",")]
            if len(parts) == self.model.num_pixels:
                self.model.phys_to_logical = parts

        if line.startswith("OK HELLO"):
            self._send("CUR?")
            self._send("MAP?")

        if line.startswith("RTC ") or "RTC=" in line or "TIME=" in line:
            self.rtc_label.setText(f"RTC: {line}")

    def _assign_next(self) -> None:
        logical = int(self.logical_box.currentData())
        assign_phys = self.cur_phys

        current_value = self.model.phys_to_logical[assign_phys]
        if (
            logical != LOG_UNUSED
            and logical in self.model.used_logicals()
            and current_value != logical
        ):
            self.log.appendPlainText("!! Logical already used; choose another.")
            return

        self.model.assign(assign_phys, logical)
        self._send(f"ASSIGN {logical}")

        self.cur_phys = (assign_phys + 1) % self.model.num_pixels
        self.current_phys_label.setText(f"Current physical LED: {self.cur_phys}")

        self._send("NEXT")
        self._send("CUR?")

    def _save_json(self) -> None:
        path, _ = QtWidgets.QFileDialog.getSaveFileName(
            self,
            "Save JSON",
            "ledmap.json",
            "JSON (*.json)",
        )
        if not path:
            return
        Path(path).write_text(json.dumps(self.model.to_json_obj(), indent=2), encoding="utf-8")
        errs = self.model.validate()
        self.log.appendPlainText("Saved JSON. Validate: " + ("OK" if not errs else "; ".join(errs)))

    def _save_h(self) -> None:
        path, _ = QtWidgets.QFileDialog.getSaveFileName(
            self,
            "Export Header",
            "Generated_LedMap.h",
            "Header (*.h)",
        )
        if not path:
            return
        Path(path).write_text(self.model.export_header(), encoding="utf-8")
        self.log.appendPlainText("Exported header.")

    def _send_mode(self) -> None:
        self._send(f"MODE {self.mode_box.currentText()}")

    def _send_hour_mode(self) -> None:
        self._send(f"HOURMODE {self.hour_mode_box.currentText()}")

    def _send_seconds_mode(self) -> None:
        self._send(f"SECONDSMODE {self.seconds_mode_box.currentText()}")

    def _send_colon_brightness(self) -> None:
        self._send(f"COLONBRIGHTNESS {self.colon_brightness_slider.value()}")

    def _send_brightness(self) -> None:
        self._send(f"BRIGHTNESS {self.brightness_slider.value()}")

    def _send_color(self, name: str, button: ColorButton) -> None:
        r, g, b = button.rgb_tuple()
        self._send(f"COLOR {name} {r} {g} {b}")

    def _send_all_colors(self) -> None:
        self._send_color("CLOCK", self.clock_color_btn)
        self._send_color("ACCENT", self.accent_color_btn)
        self._send_color("SECONDS", self.seconds_color_btn)
        self._send_color("TIMER", self.timer_color_btn)

    def _send_accent_color(self) -> None:
        self._send_color("ACCENT", self.accent_color_btn)

    def _send_hours_color(self) -> None:
        self._send_color("HOURS", self.hours_color_btn)

    def _send_minutes_color(self) -> None:
        self._send_color("MINUTES", self.minutes_color_btn)

    def _send_digit_colors(self) -> None:
        self._send_color("DIGIT1", self.digit1_color_btn)
        self._send_color("DIGIT2", self.digit2_color_btn)
        self._send_color("DIGIT3", self.digit3_color_btn)
        self._send_color("DIGIT4", self.digit4_color_btn)

    def _apply_color_preset(self) -> None:
        presets = {
            "Warm Classic": {
                "clock": QtGui.QColor(255, 120, 40),
                "hours": QtGui.QColor(255, 120, 40),
                "minutes": QtGui.QColor(255, 120, 40),
                "digit1": QtGui.QColor(255, 120, 40),
                "digit2": QtGui.QColor(255, 120, 40),
                "digit3": QtGui.QColor(255, 120, 40),
                "digit4": QtGui.QColor(255, 120, 40),
                "accent": QtGui.QColor(0, 120, 255),
                "seconds": QtGui.QColor(255, 255, 255),
                "timer": QtGui.QColor(80, 255, 120),
                "preset_cmd": None,
            },
            "Ice Blue": {
                "clock": QtGui.QColor(120, 220, 255),
                "hours": QtGui.QColor(120, 220, 255),
                "minutes": QtGui.QColor(120, 220, 255),
                "digit1": QtGui.QColor(120, 220, 255),
                "digit2": QtGui.QColor(120, 220, 255),
                "digit3": QtGui.QColor(120, 220, 255),
                "digit4": QtGui.QColor(120, 220, 255),
                "accent": QtGui.QColor(0, 80, 255),
                "seconds": QtGui.QColor(220, 245, 255),
                "timer": QtGui.QColor(80, 255, 220),
                "preset_cmd": None,
            },
            "Matrix": {
                "clock": QtGui.QColor(0, 255, 80),
                "hours": QtGui.QColor(0, 255, 80),
                "minutes": QtGui.QColor(0, 255, 80),
                "digit1": QtGui.QColor(0, 255, 80),
                "digit2": QtGui.QColor(0, 255, 80),
                "digit3": QtGui.QColor(0, 255, 80),
                "digit4": QtGui.QColor(0, 255, 80),
                "accent": QtGui.QColor(0, 120, 40),
                "seconds": QtGui.QColor(180, 255, 180),
                "timer": QtGui.QColor(80, 255, 120),
                "preset_cmd": None,
            },
            "Sunset": {
                "clock": QtGui.QColor(255, 90, 40),
                "hours": QtGui.QColor(255, 90, 40),
                "minutes": QtGui.QColor(255, 90, 40),
                "digit1": QtGui.QColor(255, 90, 40),
                "digit2": QtGui.QColor(255, 90, 40),
                "digit3": QtGui.QColor(255, 90, 40),
                "digit4": QtGui.QColor(255, 90, 40),
                "accent": QtGui.QColor(255, 0, 120),
                "seconds": QtGui.QColor(255, 220, 120),
                "timer": QtGui.QColor(255, 180, 80),
                "preset_cmd": None,
            },
            "Rainbow": {
                "clock": QtGui.QColor(255, 0, 0),
                "hours": QtGui.QColor(255, 60, 0),
                "minutes": QtGui.QColor(0, 120, 255),
                "digit1": QtGui.QColor(255, 0, 0),
                "digit2": QtGui.QColor(255, 120, 0),
                "digit3": QtGui.QColor(0, 255, 0),
                "digit4": QtGui.QColor(0, 120, 255),
                "accent": QtGui.QColor(180, 0, 255),
                "seconds": QtGui.QColor(0, 120, 255),
                "timer": QtGui.QColor(180, 0, 255),
                "preset_cmd": "PRESET RAINBOW",
            },
            "Party": {
                "clock": QtGui.QColor(255, 0, 255),
                "hours": QtGui.QColor(255, 0, 255),
                "minutes": QtGui.QColor(255, 0, 255),
                "digit1": QtGui.QColor(255, 0, 255),
                "digit2": QtGui.QColor(255, 0, 255),
                "digit3": QtGui.QColor(255, 0, 255),
                "digit4": QtGui.QColor(255, 0, 255),
                "accent": QtGui.QColor(0, 255, 255),
                "seconds": QtGui.QColor(255, 255, 0),
                "timer": QtGui.QColor(255, 80, 80),
                "preset_cmd": "PRESET PARTY",
            },
        }

        preset = presets[self.color_preset_box.currentText()]
        self.clock_color_btn.set_color(preset["clock"])
        self.hours_color_btn.set_color(preset["hours"])
        self.minutes_color_btn.set_color(preset["minutes"])
        self.digit1_color_btn.set_color(preset["digit1"])
        self.digit2_color_btn.set_color(preset["digit2"])
        self.digit3_color_btn.set_color(preset["digit3"])
        self.digit4_color_btn.set_color(preset["digit4"])
        self.accent_color_btn.set_color(preset["accent"])
        self.seconds_color_btn.set_color(preset["seconds"])
        self.timer_color_btn.set_color(preset["timer"])

        if preset["preset_cmd"] is not None:
            self._send(preset["preset_cmd"])
        else:
            self._send_all_colors()
            self._send_hours_color()
            self._send_minutes_color()
            self._send_digit_colors()

    def _set_rtc_from_pc(self) -> None:
        now = datetime.now()
        self._send(
            f"RTC SET {now.year:04d} {now.month:02d} {now.day:02d} {now.hour:02d} {now.minute:02d} {now.second:02d}"
        )
        self.datetime_edit.setDateTime(QtCore.QDateTime.currentDateTime())

    def _set_rtc_manual(self) -> None:
        dt = self.datetime_edit.dateTime().toPython()
        self._send(
            f"RTC SET {dt.year:04d} {dt.month:02d} {dt.day:02d} {dt.hour:02d} {dt.minute:02d} {dt.second:02d}"
        )

    def _set_timer(self) -> None:
        total_seconds = self.timer_minutes.value() * 60 + self.timer_seconds.value()
        self._send(f"TIMER SET {total_seconds}")

    def _send_custom_command(self) -> None:
        command = self.custom_command.text().strip()
        if not command:
            return
        self._send(command)
        self.custom_command.clear()


    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self.thread.quit()
        self.thread.wait(1000)
        super().closeEvent(event)

def main() -> None:
    app = QtWidgets.QApplication(sys.argv)
    w = MainWindow()
    w.resize(1460, 760)
    w.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
