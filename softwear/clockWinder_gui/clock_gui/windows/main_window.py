from __future__ import annotations

from datetime import datetime
from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import (
    APP_NAME,
    BASE_COLOR_TARGETS,
    COLOR_PRESETS,
    COLOR_TARGETS,
    DEFAULT_EXPORT_DIR,
    HOUR_MODES,
    LOG_UNUSED,
    MODES,
    SECONDS_MODES,
    SERIAL_HANDSHAKE_DELAY_MS,
    STOPWATCH_COLOR_TARGETS,
    TIMER_COLOR_TARGETS,
    TIMER_FORMATS,
    LogicalId,
    logical_name,
    logical_picker_label,
)
from ..exports import load_mapping_json, save_mapping_header, save_mapping_json
from ..model import DeviceHello, DeviceRtc, DeviceStatus, MappingModel
from ..protocol import ClockProtocol
from ..serial_worker import SerialWorker
from ..widgets import ColorButton, LogPanel, MappingTable, StatusPanel


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self) -> None:
        super().__init__()
        self.setWindowTitle(APP_NAME)
        self.resize(1480, 920)
        self.setMinimumSize(1180, 760)

        self.model = MappingModel()
        self.device_hello = DeviceHello()
        self.device_status = DeviceStatus()
        self.device_rtc = DeviceRtc()
        self.connected = False
        self.current_phys = 0
        self.current_port = ""
        self._build_ui()
        self._build_worker()
        self._refresh_mapping_views()
        self._refresh_status_panel()
        self._refresh_connection_buttons()
        self._queue_worker_call("refresh_ports")

    def _build_ui(self) -> None:
        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        root = QtWidgets.QVBoxLayout(central)
        root.setContentsMargins(10, 10, 10, 10)
        root.setSpacing(8)

        root.addWidget(self._build_connection_bar())

        outer_splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        outer_splitter.setChildrenCollapsible(False)
        root.addWidget(outer_splitter, 1)

        top_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        top_splitter.setChildrenCollapsible(False)
        outer_splitter.addWidget(top_splitter)

        controls_container = QtWidgets.QWidget()
        controls_layout = QtWidgets.QVBoxLayout(controls_container)
        controls_layout.setContentsMargins(0, 0, 0, 0)
        controls_layout.setSpacing(8)

        controls_scroll = QtWidgets.QScrollArea()
        controls_scroll.setWidgetResizable(True)
        controls_scroll.setFrameShape(QtWidgets.QFrame.NoFrame)
        controls_scroll.setWidget(controls_container)
        top_splitter.addWidget(controls_scroll)

        right_splitter = QtWidgets.QSplitter(QtCore.Qt.Vertical)
        right_splitter.setChildrenCollapsible(False)
        top_splitter.addWidget(right_splitter)
        top_splitter.setSizes([760, 640])

        controls_layout.addWidget(self._build_calibration_group())
        controls_layout.addWidget(self._build_clock_group())
        controls_layout.addWidget(self._build_rtc_timer_group())
        controls_layout.addStretch(1)

        self.status_panel = StatusPanel()
        right_splitter.addWidget(self.status_panel)

        mapping_group = QtWidgets.QGroupBox("Mapping Table")
        mapping_layout = QtWidgets.QVBoxLayout(mapping_group)
        mapping_layout.setContentsMargins(8, 8, 8, 8)
        self.mapping_summary_label = QtWidgets.QLabel()
        self.mapping_summary_label.setWordWrap(True)
        self.mapping_table = MappingTable()
        self.mapping_table.physical_selected.connect(self._on_mapping_row_selected)
        mapping_layout.addWidget(self.mapping_summary_label)
        mapping_layout.addWidget(self.mapping_table, 1)
        right_splitter.addWidget(mapping_group)
        right_splitter.setSizes([220, 620])

        self.log_panel = LogPanel()
        outer_splitter.addWidget(self.log_panel)
        outer_splitter.setSizes([700, 220])

        self.statusBar().showMessage("Ready")

    def _build_connection_bar(self) -> QtWidgets.QWidget:
        widget = QtWidgets.QGroupBox("Connection")
        layout = QtWidgets.QGridLayout(widget)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setHorizontalSpacing(8)
        layout.setVerticalSpacing(8)

        self.port_combo = QtWidgets.QComboBox()
        self.port_combo.setEditable(True)
        self.port_combo.setMinimumWidth(260)
        self.refresh_ports_button = QtWidgets.QPushButton("Refresh")
        self.connect_button = QtWidgets.QPushButton("Connect")
        self.disconnect_button = QtWidgets.QPushButton("Disconnect")
        self.hello_button = QtWidgets.QPushButton("HELLO Clock/1")
        self.hello_cal_button = QtWidgets.QPushButton("HELLO ClockCal/1")
        self.info_button = QtWidgets.QPushButton("INFO?")
        self.status_button = QtWidgets.QPushButton("STATUS?")

        layout.addWidget(QtWidgets.QLabel("Port"), 0, 0)
        layout.addWidget(self.port_combo, 0, 1)
        layout.addWidget(self.refresh_ports_button, 0, 2)
        layout.addWidget(self.connect_button, 0, 3)
        layout.addWidget(self.disconnect_button, 0, 4)
        layout.addWidget(self.hello_button, 1, 1)
        layout.addWidget(self.hello_cal_button, 1, 2)
        layout.addWidget(self.info_button, 1, 3)
        layout.addWidget(self.status_button, 1, 4)

        self.refresh_ports_button.clicked.connect(
            lambda: self._queue_worker_call("refresh_ports")
        )
        self.connect_button.clicked.connect(self._connect_selected_port)
        self.disconnect_button.clicked.connect(
            lambda: self._queue_worker_call("disconnect_port")
        )
        self.hello_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.hello("Clock/1"))
        )
        self.hello_cal_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.hello("ClockCal/1"))
        )
        self.info_button.clicked.connect(lambda: self._send_command(ClockProtocol.info()))
        self.status_button.clicked.connect(lambda: self._send_command(ClockProtocol.status()))

        return widget

    def _build_calibration_group(self) -> QtWidgets.QGroupBox:
        group = QtWidgets.QGroupBox("Calibration Workflow")
        layout = QtWidgets.QVBoxLayout(group)
        layout.setSpacing(8)

        nav_layout = QtWidgets.QGridLayout()
        self.mode_cal_button = QtWidgets.QPushButton("MODE CAL")
        self.mode_run_button = QtWidgets.QPushButton("MODE RUN")
        self.current_button = QtWidgets.QPushButton("CUR?")
        self.map_button = QtWidgets.QPushButton("MAP?")
        self.prev_button = QtWidgets.QPushButton("Prev")
        self.next_button = QtWidgets.QPushButton("Next")
        self.set_selected_button = QtWidgets.QPushButton("Set Selected LED")
        self.validate_button = QtWidgets.QPushButton("Validate")
        self.test_segments_button = QtWidgets.QPushButton("Test Segments")
        self.test_digits_button = QtWidgets.QPushButton("Test Digits")
        self.test_all_button = QtWidgets.QPushButton("Test All")
        self.save_device_button = QtWidgets.QPushButton("Save Device Map")
        self.load_device_button = QtWidgets.QPushButton("Load Device Map")

        nav_layout.addWidget(self.mode_cal_button, 0, 0)
        nav_layout.addWidget(self.mode_run_button, 0, 1)
        nav_layout.addWidget(self.current_button, 0, 2)
        nav_layout.addWidget(self.map_button, 0, 3)
        nav_layout.addWidget(self.prev_button, 1, 0)
        nav_layout.addWidget(self.next_button, 1, 1)
        nav_layout.addWidget(self.set_selected_button, 1, 2)
        nav_layout.addWidget(self.validate_button, 1, 3)
        nav_layout.addWidget(self.test_segments_button, 2, 0)
        nav_layout.addWidget(self.test_digits_button, 2, 1)
        nav_layout.addWidget(self.test_all_button, 2, 2)
        nav_layout.addWidget(self.save_device_button, 3, 0)
        nav_layout.addWidget(self.load_device_button, 3, 1)
        layout.addLayout(nav_layout)

        picker_row = QtWidgets.QHBoxLayout()
        self.logical_picker = QtWidgets.QComboBox()
        self.logical_picker.setMinimumWidth(240)
        self._rebuild_logical_picker()
        self.assign_current_button = QtWidgets.QPushButton("Assign To Current")
        self.assign_and_next_button = QtWidgets.QPushButton("Assign + Next")
        self.unassign_current_button = QtWidgets.QPushButton("Unassign Current")
        picker_row.addWidget(QtWidgets.QLabel("Logical Target"))
        picker_row.addWidget(self.logical_picker, 1)
        picker_row.addWidget(self.assign_current_button)
        picker_row.addWidget(self.assign_and_next_button)
        picker_row.addWidget(self.unassign_current_button)
        layout.addLayout(picker_row)

        export_row = QtWidgets.QHBoxLayout()
        self.import_json_button = QtWidgets.QPushButton("Import JSON")
        self.export_json_button = QtWidgets.QPushButton("Export JSON")
        self.export_header_button = QtWidgets.QPushButton("Export Header")
        export_row.addWidget(self.import_json_button)
        export_row.addWidget(self.export_json_button)
        export_row.addWidget(self.export_header_button)
        layout.addLayout(export_row)

        self.mode_cal_button.clicked.connect(lambda: self._send_command(ClockProtocol.mode_cal()))
        self.mode_run_button.clicked.connect(lambda: self._send_command(ClockProtocol.mode_run()))
        self.current_button.clicked.connect(lambda: self._send_command(ClockProtocol.current()))
        self.map_button.clicked.connect(lambda: self._send_command(ClockProtocol.mapping()))
        self.prev_button.clicked.connect(lambda: self._send_command(ClockProtocol.prev()))
        self.next_button.clicked.connect(lambda: self._send_command(ClockProtocol.next()))
        self.set_selected_button.clicked.connect(self._set_device_to_selected_row)
        self.validate_button.clicked.connect(self._validate_mapping)
        self.test_segments_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.test_segments())
        )
        self.test_digits_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.test_digits())
        )
        self.test_all_button.clicked.connect(lambda: self._send_command(ClockProtocol.test_all()))
        self.save_device_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.save_map())
        )
        self.load_device_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.load_map())
        )
        self.assign_current_button.clicked.connect(lambda: self._assign_current(False))
        self.assign_and_next_button.clicked.connect(lambda: self._assign_current(True))
        self.unassign_current_button.clicked.connect(self._unassign_current)
        self.import_json_button.clicked.connect(self._import_json)
        self.export_json_button.clicked.connect(self._export_json)
        self.export_header_button.clicked.connect(self._export_header)

        return group

    def _build_clock_group(self) -> QtWidgets.QGroupBox:
        group = QtWidgets.QGroupBox("Clock Control")
        layout = QtWidgets.QVBoxLayout(group)
        layout.setSpacing(8)

        mode_row = QtWidgets.QHBoxLayout()
        self.mode_combo = QtWidgets.QComboBox()
        self.mode_combo.addItems(MODES)
        self.mode_send_button = QtWidgets.QPushButton("Set Mode")
        self.hour_mode_combo = QtWidgets.QComboBox()
        self.hour_mode_combo.addItems(HOUR_MODES)
        self.hour_mode_send_button = QtWidgets.QPushButton("Set Hour Mode")
        self.seconds_mode_combo = QtWidgets.QComboBox()
        self.seconds_mode_combo.addItems(SECONDS_MODES)
        self.seconds_mode_send_button = QtWidgets.QPushButton("Set Colon Mode")
        mode_row.addWidget(QtWidgets.QLabel("Mode"))
        mode_row.addWidget(self.mode_combo)
        mode_row.addWidget(self.mode_send_button)
        mode_row.addSpacing(12)
        mode_row.addWidget(QtWidgets.QLabel("Hour"))
        mode_row.addWidget(self.hour_mode_combo)
        mode_row.addWidget(self.hour_mode_send_button)
        mode_row.addSpacing(12)
        mode_row.addWidget(QtWidgets.QLabel("Colon"))
        mode_row.addWidget(self.seconds_mode_combo)
        mode_row.addWidget(self.seconds_mode_send_button)
        layout.addLayout(mode_row)

        brightness_row = QtWidgets.QGridLayout()
        self.brightness_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.brightness_slider.setRange(1, 255)
        self.brightness_value_label = QtWidgets.QLabel("64")
        self.brightness_send_button = QtWidgets.QPushButton("Send Brightness")
        self.colon_brightness_slider = QtWidgets.QSlider(QtCore.Qt.Horizontal)
        self.colon_brightness_slider.setRange(0, 255)
        self.colon_brightness_value_label = QtWidgets.QLabel("255")
        self.colon_brightness_send_button = QtWidgets.QPushButton("Send Colon Brightness")
        self.brightness_slider.valueChanged.connect(
            lambda value: self.brightness_value_label.setText(str(value))
        )
        self.colon_brightness_slider.valueChanged.connect(
            lambda value: self.colon_brightness_value_label.setText(str(value))
        )
        brightness_row.addWidget(QtWidgets.QLabel("Brightness"), 0, 0)
        brightness_row.addWidget(self.brightness_slider, 0, 1)
        brightness_row.addWidget(self.brightness_value_label, 0, 2)
        brightness_row.addWidget(self.brightness_send_button, 0, 3)
        brightness_row.addWidget(QtWidgets.QLabel("Colon Brightness"), 1, 0)
        brightness_row.addWidget(self.colon_brightness_slider, 1, 1)
        brightness_row.addWidget(self.colon_brightness_value_label, 1, 2)
        brightness_row.addWidget(self.colon_brightness_send_button, 1, 3)
        layout.addLayout(brightness_row)

        preset_row = QtWidgets.QHBoxLayout()
        self.preset_combo = QtWidgets.QComboBox()
        self.preset_combo.addItems(COLOR_PRESETS.keys())
        self.preset_send_button = QtWidgets.QPushButton("Apply Preset")
        preset_row.addWidget(QtWidgets.QLabel("Preset"))
        preset_row.addWidget(self.preset_combo, 1)
        preset_row.addWidget(self.preset_send_button)
        layout.addLayout(preset_row)

        self.color_buttons: dict[str, ColorButton] = {}
        color_grid = QtWidgets.QGridLayout()
        for index, target in enumerate(BASE_COLOR_TARGETS):
            button = ColorButton(target.title(), COLOR_PRESETS["Warm Classic"].get(target, (0, 0, 0)))
            self.color_buttons[target] = button
            color_grid.addWidget(button, index // 2, index % 2)
        layout.addLayout(color_grid)

        color_send_row = QtWidgets.QHBoxLayout()
        self.send_basic_colors_button = QtWidgets.QPushButton("Send Base Colors")
        self.send_group_colors_button = QtWidgets.QPushButton("Send Hour/Minute Colors")
        self.send_digit_colors_button = QtWidgets.QPushButton("Send Digit Colors")
        color_send_row.addWidget(self.send_basic_colors_button)
        color_send_row.addWidget(self.send_group_colors_button)
        color_send_row.addWidget(self.send_digit_colors_button)
        layout.addLayout(color_send_row)

        self.mode_send_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.set_mode(self.mode_combo.currentText()))
        )
        self.hour_mode_send_button.clicked.connect(
            lambda: self._send_command(
                ClockProtocol.set_hour_mode(self.hour_mode_combo.currentText())
            )
        )
        self.seconds_mode_send_button.clicked.connect(
            lambda: self._send_command(
                ClockProtocol.set_seconds_mode(self.seconds_mode_combo.currentText())
            )
        )
        self.brightness_send_button.clicked.connect(
            lambda: self._send_command(
                ClockProtocol.set_brightness(self.brightness_slider.value())
            )
        )
        self.colon_brightness_send_button.clicked.connect(
            lambda: self._send_command(
                ClockProtocol.set_colon_brightness(self.colon_brightness_slider.value())
            )
        )
        self.preset_send_button.clicked.connect(self._apply_preset)
        self.send_basic_colors_button.clicked.connect(self._send_basic_colors)
        self.send_group_colors_button.clicked.connect(self._send_group_colors)
        self.send_digit_colors_button.clicked.connect(self._send_digit_colors)

        return group

    def _build_rtc_timer_group(self) -> QtWidgets.QGroupBox:
        group = QtWidgets.QGroupBox("RTC, Timer, Stopwatch, Raw Command")
        layout = QtWidgets.QVBoxLayout(group)
        layout.setSpacing(8)

        rtc_group = QtWidgets.QGroupBox("RTC")
        rtc_layout = QtWidgets.QGridLayout(rtc_group)
        self.read_rtc_button = QtWidgets.QPushButton("Read RTC")
        self.set_rtc_now_button = QtWidgets.QPushButton("Set RTC From PC")
        self.rtc_datetime_edit = QtWidgets.QDateTimeEdit(QtCore.QDateTime.currentDateTime())
        self.rtc_datetime_edit.setCalendarPopup(True)
        self.set_rtc_manual_button = QtWidgets.QPushButton("Set RTC Manual")
        rtc_layout.addWidget(self.read_rtc_button, 0, 0)
        rtc_layout.addWidget(self.set_rtc_now_button, 0, 1)
        rtc_layout.addWidget(self.rtc_datetime_edit, 1, 0)
        rtc_layout.addWidget(self.set_rtc_manual_button, 1, 1)
        layout.addWidget(rtc_group)

        timer_group = QtWidgets.QGroupBox("Timer")
        timer_layout = QtWidgets.QGridLayout(timer_group)
        self.timer_hours = QtWidgets.QSpinBox()
        self.timer_hours.setRange(0, 99)
        self.timer_minutes = QtWidgets.QSpinBox()
        self.timer_minutes.setRange(0, 59)
        self.timer_minutes.setValue(5)
        self.timer_seconds = QtWidgets.QSpinBox()
        self.timer_seconds.setRange(0, 59)
        self.timer_format_combo = QtWidgets.QComboBox()
        for label, value in TIMER_FORMATS:
            self.timer_format_combo.addItem(label, value)
        self.timer_set_button = QtWidgets.QPushButton("Set Timer")
        self.timer_start_button = QtWidgets.QPushButton("Start")
        self.timer_stop_button = QtWidgets.QPushButton("Stop")
        self.timer_reset_button = QtWidgets.QPushButton("Reset")
        self.timer_color_buttons: dict[str, ColorButton] = {}
        for index, target in enumerate(TIMER_COLOR_TARGETS):
            title = target.replace("TIMER_", "Timer ").replace("_", " ").title()
            button = ColorButton(title, COLOR_PRESETS["Warm Classic"].get(target, (80, 255, 120)))
            self.color_buttons[target] = button
            self.timer_color_buttons[target] = button
            timer_layout.addWidget(button, 2 + index // 2, index % 2, 1, 2)
        self.timer_colors_send_button = QtWidgets.QPushButton("Send Timer Colors")
        timer_layout.addWidget(QtWidgets.QLabel("Hours"), 0, 0)
        timer_layout.addWidget(self.timer_hours, 0, 1)
        timer_layout.addWidget(QtWidgets.QLabel("Minutes"), 0, 2)
        timer_layout.addWidget(self.timer_minutes, 0, 3)
        timer_layout.addWidget(QtWidgets.QLabel("Seconds"), 0, 4)
        timer_layout.addWidget(self.timer_seconds, 0, 5)
        timer_layout.addWidget(self.timer_set_button, 0, 6)
        timer_layout.addWidget(QtWidgets.QLabel("Display"), 1, 0)
        timer_layout.addWidget(self.timer_format_combo, 1, 1, 1, 2)
        self.timer_format_send_button = QtWidgets.QPushButton("Send Timer Format")
        timer_layout.addWidget(self.timer_format_send_button, 1, 3, 1, 2)
        timer_layout.addWidget(self.timer_start_button, 1, 5)
        timer_layout.addWidget(self.timer_stop_button, 1, 6)
        timer_layout.addWidget(self.timer_reset_button, 2, 6)
        timer_layout.addWidget(self.timer_colors_send_button, 3, 4, 1, 3)
        layout.addWidget(timer_group)

        stopwatch_group = QtWidgets.QGroupBox("Stopwatch")
        stopwatch_layout = QtWidgets.QGridLayout(stopwatch_group)
        self.stopwatch_start_button = QtWidgets.QPushButton("Start")
        self.stopwatch_stop_button = QtWidgets.QPushButton("Stop")
        self.stopwatch_reset_button = QtWidgets.QPushButton("Reset")
        self.stopwatch_color_buttons: dict[str, ColorButton] = {}
        for index, target in enumerate(STOPWATCH_COLOR_TARGETS):
            title = target.replace("STOPWATCH_", "Stopwatch ").replace("_", " ").title()
            button = ColorButton(title, COLOR_PRESETS["Warm Classic"].get(target, (255, 120, 40)))
            self.color_buttons[target] = button
            self.stopwatch_color_buttons[target] = button
            stopwatch_layout.addWidget(button, 0, index)
        self.stopwatch_colors_send_button = QtWidgets.QPushButton("Send Stopwatch Colors")
        stopwatch_layout.addWidget(self.stopwatch_start_button, 1, 0)
        stopwatch_layout.addWidget(self.stopwatch_stop_button, 1, 1)
        stopwatch_layout.addWidget(self.stopwatch_reset_button, 1, 2)
        stopwatch_layout.addWidget(self.stopwatch_colors_send_button, 1, 3)
        layout.addWidget(stopwatch_group)

        raw_group = QtWidgets.QGroupBox("Raw Serial Command")
        raw_layout = QtWidgets.QHBoxLayout(raw_group)
        self.raw_command_edit = QtWidgets.QLineEdit()
        self.raw_command_edit.setPlaceholderText("Send a line-based command to the firmware")
        self.raw_send_button = QtWidgets.QPushButton("Send")
        raw_layout.addWidget(self.raw_command_edit, 1)
        raw_layout.addWidget(self.raw_send_button)
        layout.addWidget(raw_group)

        self.read_rtc_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.rtc_read())
        )
        self.set_rtc_now_button.clicked.connect(self._set_rtc_from_pc)
        self.set_rtc_manual_button.clicked.connect(self._set_rtc_manual)
        self.timer_set_button.clicked.connect(self._set_timer)
        self.timer_format_send_button.clicked.connect(
            lambda: self._send_command(
                ClockProtocol.set_timer_format(str(self.timer_format_combo.currentData()))
            )
        )
        self.timer_start_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.timer_start())
        )
        self.timer_stop_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.timer_stop())
        )
        self.timer_reset_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.timer_reset())
        )
        self.stopwatch_start_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.stopwatch_start())
        )
        self.stopwatch_stop_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.stopwatch_stop())
        )
        self.stopwatch_reset_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.stopwatch_reset())
        )
        self.timer_colors_send_button.clicked.connect(self._send_timer_colors)
        self.stopwatch_colors_send_button.clicked.connect(self._send_stopwatch_colors)
        self.raw_send_button.clicked.connect(self._send_raw_command)
        self.raw_command_edit.returnPressed.connect(self._send_raw_command)

        return group

    def _build_worker(self) -> None:
        self.worker = SerialWorker()
        self.worker_thread = QtCore.QThread(self)
        self.worker.moveToThread(self.worker_thread)
        self.worker_thread.started.connect(self.worker.start)
        self.worker.ports_changed.connect(self._on_ports_changed)
        self.worker.connection_changed.connect(self._on_connection_changed)
        self.worker.line_received.connect(self._on_line_received)
        self.worker.worker_log.connect(self._append_log)
        self.worker_thread.start()

    def _queue_worker_call(self, method_name: str, argument: str | None = None) -> None:
        if argument is None:
            QtCore.QMetaObject.invokeMethod(
                self.worker,
                method_name,
                QtCore.Qt.QueuedConnection,
            )
        else:
            QtCore.QMetaObject.invokeMethod(
                self.worker,
                method_name,
                QtCore.Qt.QueuedConnection,
                QtCore.Q_ARG(str, argument),
            )

    def _connect_selected_port(self) -> None:
        port_name = self.port_combo.currentText().strip()
        self.current_port = port_name
        self._queue_worker_call("connect_port", port_name)

    def _send_command(self, command: str) -> None:
        if not command:
            return
        self._append_log(f">> {command}")
        self._queue_worker_call("send_line", command)

    def _send_commands(self, commands: list[str], start_delay_ms: int = 0, step_ms: int = 110) -> None:
        for index, command in enumerate(commands):
            QtCore.QTimer.singleShot(
                start_delay_ms + index * step_ms,
                lambda cmd=command: self._send_command(cmd),
            )

    def _append_log(self, line: str) -> None:
        self.log_panel.append(line)

    def _on_ports_changed(self, ports: list[str]) -> None:
        current_text = self.port_combo.currentText()
        self.port_combo.blockSignals(True)
        self.port_combo.clear()
        self.port_combo.addItems(ports)
        if current_text:
            if current_text not in ports:
                self.port_combo.addItem(current_text)
            self.port_combo.setCurrentText(current_text)
        self.port_combo.blockSignals(False)
        self.statusBar().showMessage(f"Found {len(ports)} serial port(s)", 3000)

    def _on_connection_changed(self, connected: bool, message: str) -> None:
        self.connected = connected
        self._append_log(message)
        self.statusBar().showMessage(message, 5000)
        if connected:
            self.current_port = self.port_combo.currentText().strip()
            self._send_commands(
                [
                    ClockProtocol.hello("Clock/1"),
                    ClockProtocol.info(),
                    ClockProtocol.status(),
                    ClockProtocol.current(),
                    ClockProtocol.mapping(),
                    ClockProtocol.rtc_read(),
                ],
                start_delay_ms=SERIAL_HANDSHAKE_DELAY_MS,
            )
        self._refresh_status_panel(connection_message=message)
        self._refresh_connection_buttons()

    def _on_line_received(self, line: str) -> None:
        self._append_log(f"<< {line}")
        parsed = ClockProtocol.parse_line(line)

        if parsed.kind == "hello":
            self.device_hello.protocol = parsed.data.get("protocol", "")
            self.device_hello.name = parsed.data.get("NAME", self.device_hello.name)
            self.device_hello.firmware_version = parsed.data.get(
                "FW", self.device_hello.firmware_version
            )
            self.device_hello.pixels = int(parsed.data.get("pixels", self.device_hello.pixels))
            self.device_hello.logical_count = int(
                parsed.data.get("logical", self.device_hello.logical_count)
            )
            self.device_hello.calibration_mode = bool(
                parsed.data.get("calibration_mode", self.device_hello.calibration_mode)
            )
            self.device_hello.rtc_present = parsed.data.get(
                "rtc_present", self.device_hello.rtc_present
            )

        elif parsed.kind == "info":
            self.device_hello.board = parsed.data.get("BOARD", self.device_hello.board)
            self.device_hello.pin = parsed.data.get("PIN", self.device_hello.pin)
            self.device_hello.buttons = parsed.data.get("BTNS", self.device_hello.buttons)

        elif parsed.kind == "status":
            self.device_status.mode = parsed.data.get("MODE", self.device_status.mode)
            self.device_status.hour_mode = parsed.data.get(
                "HOURMODE", self.device_status.hour_mode
            )
            self.device_status.seconds_mode = parsed.data.get(
                "SECONDSMODE", self.device_status.seconds_mode
            )
            self.device_status.timer_format = parsed.data.get(
                "timer_format", self.device_status.timer_format
            )
            self.device_status.brightness = int(
                parsed.data.get("brightness", self.device_status.brightness)
            )
            self.device_status.colon_brightness = int(
                parsed.data.get(
                    "colon_brightness", self.device_status.colon_brightness
                )
            )
            self.device_status.rainbow = bool(
                parsed.data.get("rainbow", self.device_status.rainbow)
            )
            self.device_status.timer_preset = int(
                parsed.data.get("timer_preset", self.device_status.timer_preset)
            )
            self.device_status.timer_remaining = int(
                parsed.data.get("timer_remaining", self.device_status.timer_remaining)
            )
            self.device_status.timer_running = bool(
                parsed.data.get("timer_running", self.device_status.timer_running)
            )
            self.device_status.stopwatch_running = bool(
                parsed.data.get(
                    "stopwatch_running", self.device_status.stopwatch_running
                )
            )
            self.device_status.calibration_mode = bool(
                parsed.data.get(
                    "calibration_mode", self.device_status.calibration_mode
                )
            )
            self._sync_controls_from_status()

        elif parsed.kind == "current":
            self.current_phys = int(parsed.data.get("current_phys", self.current_phys))

        elif parsed.kind == "map":
            values = list(parsed.data.get("values", []))
            if values:
                try:
                    self.model.apply_map(values)
                except ValueError as exc:
                    self._append_log(f"!! {exc}")
                self._rebuild_logical_picker()

        elif parsed.kind == "rtc":
            self.device_rtc.summary = str(parsed.data.get("summary", self.device_rtc.summary))
            self.device_rtc.lost_power = parsed.data.get(
                "lost_power", self.device_rtc.lost_power
            )
            self.device_rtc.is_missing = bool(
                parsed.data.get("is_missing", self.device_rtc.is_missing)
            )

        elif parsed.kind == "ok":
            if parsed.data.get("message") == "TIMER DONE":
                self.statusBar().showMessage("Timer completed", 4000)
            if parsed.data.get("message", "").startswith("VALIDATE "):
                self.statusBar().showMessage(parsed.data["message"], 4000)

        elif parsed.kind == "error":
            self.statusBar().showMessage(parsed.data.get("message", "Device error"), 5000)

        self._refresh_mapping_views()
        self._refresh_status_panel()

    def _refresh_mapping_views(self) -> None:
        self.mapping_summary_label.setText(self.model.summary())
        self.mapping_table.set_mapping(self.model, self.current_phys)

    def _refresh_status_panel(self, connection_message: str | None = None) -> None:
        if connection_message is None:
            connection_message = "Connected" if self.connected else "Disconnected"
        self.status_panel.set_value("connection", connection_message)
        self.status_panel.set_value("port", self.current_port or "—")
        protocol = self.device_hello.protocol or "—"
        if self.device_hello.calibration_mode or self.device_status.calibration_mode:
            protocol += " (calibration active)"
        self.status_panel.set_value("protocol", protocol)
        firmware = self.device_hello.name or "—"
        if self.device_hello.firmware_version:
            firmware = f"{firmware} {self.device_hello.firmware_version}".strip()
        self.status_panel.set_value("firmware", firmware)
        board = self.device_hello.board or "Leonardo / unknown"
        if self.device_hello.pin or self.device_hello.buttons:
            extras = []
            if self.device_hello.pin:
                extras.append(f"D{self.device_hello.pin}")
            if self.device_hello.buttons:
                extras.append(f"Buttons {self.device_hello.buttons}")
            board = f"{board} ({', '.join(extras)})"
        self.status_panel.set_value("board", board)
        self.status_panel.set_value(
            "mode",
            f"{self.device_status.mode}, {self.device_status.hour_mode}, "
            f"{self.device_status.seconds_mode}, timer {self.device_status.timer_format}",
        )
        self.status_panel.set_value(
            "brightness",
            f"{self.device_status.brightness}, colon {self.device_status.colon_brightness}",
        )
        self.status_panel.set_value(
            "timer",
            f"preset {self.device_status.timer_preset}s, remaining {self.device_status.timer_remaining}s, "
            f"{'running' if self.device_status.timer_running else 'stopped'}",
        )
        self.status_panel.set_value(
            "stopwatch",
            "running" if self.device_status.stopwatch_running else "stopped",
        )
        rtc_text = self.device_rtc.summary
        if self.device_rtc.is_missing:
            rtc_text = "missing"
        elif self.device_rtc.lost_power is not None:
            rtc_text = f"{rtc_text} (lost power {int(self.device_rtc.lost_power)})"
        self.status_panel.set_value("rtc", rtc_text)
        self.status_panel.set_value("current_phys", str(self.current_phys))
        self.status_panel.set_value("mapping", self.model.summary())

    def _refresh_connection_buttons(self) -> None:
        self.connect_button.setEnabled(not self.connected)
        self.disconnect_button.setEnabled(self.connected)
        for button in [
            self.hello_button,
            self.hello_cal_button,
            self.info_button,
            self.status_button,
            self.mode_cal_button,
            self.mode_run_button,
            self.current_button,
            self.map_button,
            self.prev_button,
            self.next_button,
            self.set_selected_button,
            self.validate_button,
            self.test_segments_button,
            self.test_digits_button,
            self.test_all_button,
            self.save_device_button,
            self.load_device_button,
            self.assign_current_button,
            self.assign_and_next_button,
            self.unassign_current_button,
            self.mode_send_button,
            self.hour_mode_send_button,
            self.seconds_mode_send_button,
            self.brightness_send_button,
            self.colon_brightness_send_button,
            self.preset_send_button,
            self.send_basic_colors_button,
            self.send_group_colors_button,
            self.send_digit_colors_button,
            self.timer_format_send_button,
            self.timer_colors_send_button,
            self.stopwatch_colors_send_button,
            self.read_rtc_button,
            self.set_rtc_now_button,
            self.set_rtc_manual_button,
            self.timer_set_button,
            self.timer_start_button,
            self.timer_stop_button,
            self.timer_reset_button,
            self.stopwatch_start_button,
            self.stopwatch_stop_button,
            self.stopwatch_reset_button,
            self.raw_send_button,
        ]:
            button.setEnabled(self.connected)

    def _sync_controls_from_status(self) -> None:
        for combo, value in [
            (self.mode_combo, self.device_status.mode),
            (self.hour_mode_combo, self.device_status.hour_mode),
            (self.seconds_mode_combo, self.device_status.seconds_mode),
        ]:
            index = combo.findText(value)
            if index >= 0:
                combo.setCurrentIndex(index)
        self.brightness_slider.setValue(self.device_status.brightness)
        self.colon_brightness_slider.setValue(self.device_status.colon_brightness)
        total_seconds = max(self.device_status.timer_preset, 0)
        self.timer_hours.setValue(total_seconds // 3600)
        self.timer_minutes.setValue((total_seconds % 3600) // 60)
        self.timer_seconds.setValue(total_seconds % 60)
        format_index = self.timer_format_combo.findData(self.device_status.timer_format)
        if format_index >= 0:
            self.timer_format_combo.setCurrentIndex(format_index)

    def _on_mapping_row_selected(self, physical: int) -> None:
        logical = self.model.logical_for_phys(physical)
        index = self.logical_picker.findData(logical)
        if index >= 0:
            self.logical_picker.setCurrentIndex(index)

    def _validate_mapping(self) -> None:
        errors = self.model.validate()
        if errors:
            for error in errors:
                self._append_log(f"!! {error}")
            self.statusBar().showMessage("Local mapping has validation issues", 4000)
        else:
            self._append_log("Local mapping validation passed")
            self.statusBar().showMessage("Local mapping validation passed", 4000)
        self._send_command(ClockProtocol.validate())

    def _assign_current(self, move_next: bool) -> None:
        logical = int(self.logical_picker.currentData())
        if logical != LOG_UNUSED:
            duplicates = self.model.used_logicals()
            current_value = self.model.logical_for_phys(self.current_phys)
            if logical in duplicates and current_value != logical:
                self._append_log(
                    f"!! {logical_name(logical)} is already assigned to {duplicates[logical]}"
                )
                return

        self.model.assign(self.current_phys, logical)
        self._rebuild_logical_picker()
        self._refresh_mapping_views()
        self._send_command(ClockProtocol.assign(logical))
        if move_next:
            self.current_phys = (self.current_phys + 1) % self.model.num_pixels
            self._send_commands([ClockProtocol.next(), ClockProtocol.current()], step_ms=80)
        else:
            self._send_command(ClockProtocol.current())

    def _unassign_current(self) -> None:
        self.model.unassign(self.current_phys)
        self._rebuild_logical_picker()
        self._refresh_mapping_views()
        self._send_command(ClockProtocol.unassign_current())
        self._send_command(ClockProtocol.current())

    def _set_device_to_selected_row(self) -> None:
        selected = self.mapping_table.selected_physical()
        if selected is None:
            self.statusBar().showMessage("Select a row in the mapping table first", 3000)
            return
        self.current_phys = selected
        self._send_commands([ClockProtocol.set_current(selected), ClockProtocol.current()], step_ms=80)

    def _rebuild_logical_picker(self) -> None:
        current_value = self.logical_picker.currentData() if hasattr(self, "logical_picker") else LOG_UNUSED
        used = self.model.used_logicals()
        self.logical_picker.blockSignals(True)
        self.logical_picker.clear()
        self.logical_picker.addItem("UNUSED", LOG_UNUSED)
        for logical in LogicalId:
            label = logical_picker_label(int(logical))
            if logical.value in used:
                label += f" [phys {', '.join(str(item) for item in used[logical.value])}]"
            self.logical_picker.addItem(label, int(logical))
        target_index = self.logical_picker.findData(current_value)
        if target_index < 0:
            target_index = self.logical_picker.findData(
                self.model.logical_for_phys(self.current_phys)
            )
        self.logical_picker.setCurrentIndex(max(target_index, 0))
        self.logical_picker.blockSignals(False)

    def _export_json(self) -> None:
        default_path = str(DEFAULT_EXPORT_DIR / "ledmap.json")
        path, _ = QtWidgets.QFileDialog.getSaveFileName(
            self,
            "Export Mapping JSON",
            default_path,
            "JSON Files (*.json)",
        )
        if not path:
            return
        save_mapping_json(path, self.model)
        self._append_log(f"Saved JSON mapping to {path}")

    def _export_header(self) -> None:
        default_path = str(DEFAULT_EXPORT_DIR / "Generated_LedMap.h")
        path, _ = QtWidgets.QFileDialog.getSaveFileName(
            self,
            "Export Generated_LedMap.h",
            default_path,
            "Header Files (*.h)",
        )
        if not path:
            return
        save_mapping_header(path, self.model)
        self._append_log(f"Saved header mapping to {path}")

    def _import_json(self) -> None:
        path, _ = QtWidgets.QFileDialog.getOpenFileName(
            self,
            "Import Mapping JSON",
            str(DEFAULT_EXPORT_DIR),
            "JSON Files (*.json)",
        )
        if not path:
            return
        try:
            self.model = load_mapping_json(path)
        except Exception as exc:
            self._append_log(f"!! Could not import JSON: {exc}")
            return
        self._append_log(f"Imported mapping JSON from {path}")
        self._rebuild_logical_picker()
        self._refresh_mapping_views()
        self._refresh_status_panel()

    def _color(self, target: str) -> tuple[int, int, int]:
        return self.color_buttons[target].rgb()

    def _send_color_target(self, target: str) -> None:
        self._send_command(ClockProtocol.set_color(target, self._color(target)))

    def _send_basic_colors(self) -> None:
        for target in ["CLOCK", "ACCENT", "SECONDS", "TIMER"]:
            self._send_color_target(target)

    def _send_group_colors(self) -> None:
        for target in ["HOURS", "MINUTES"]:
            self._send_color_target(target)

    def _send_digit_colors(self) -> None:
        for target in ["DIGIT1", "DIGIT2", "DIGIT3", "DIGIT4"]:
            self._send_color_target(target)

    def _send_timer_colors(self) -> None:
        for target in TIMER_COLOR_TARGETS:
            self._send_color_target(target)

    def _send_stopwatch_colors(self) -> None:
        for target in STOPWATCH_COLOR_TARGETS:
            self._send_color_target(target)

    def _apply_preset(self) -> None:
        preset = COLOR_PRESETS[self.preset_combo.currentText()]
        for target in COLOR_TARGETS:
            self.color_buttons[target].set_rgb(preset[target], emit_signal=False)
        preset_cmd = preset.get("preset_cmd")
        if preset_cmd:
            self._send_command(str(preset_cmd))
        else:
            self._send_basic_colors()
            self._send_group_colors()
            self._send_digit_colors()
            self._send_timer_colors()
            self._send_stopwatch_colors()

    def _set_rtc_from_pc(self) -> None:
        now = datetime.now()
        self.rtc_datetime_edit.setDateTime(QtCore.QDateTime.currentDateTime())
        self._send_command(
            ClockProtocol.rtc_set(
                now.year, now.month, now.day, now.hour, now.minute, now.second
            )
        )

    def _set_rtc_manual(self) -> None:
        dt = self.rtc_datetime_edit.dateTime().toPython()
        self._send_command(
            ClockProtocol.rtc_set(
                dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second
            )
        )

    def _set_timer(self) -> None:
        total_seconds = (
            self.timer_hours.value() * 3600
            + self.timer_minutes.value() * 60
            + self.timer_seconds.value()
        )
        self._send_command(ClockProtocol.timer_set(total_seconds))

    def _send_raw_command(self) -> None:
        command = self.raw_command_edit.text().strip()
        if not command:
            return
        self._send_command(command)
        self.raw_command_edit.clear()

    def closeEvent(self, event: QtGui.QCloseEvent) -> None:
        self._queue_worker_call("disconnect_port")
        self.worker_thread.quit()
        self.worker_thread.wait(1000)
        super().closeEvent(event)
