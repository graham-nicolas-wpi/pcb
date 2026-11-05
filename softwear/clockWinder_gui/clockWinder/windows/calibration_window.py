from __future__ import annotations

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import (
    APP_NAME,
    DEFAULT_EXPORT_DIR,
    LOG_UNUSED,
    SERIAL_HANDSHAKE_DELAY_MS,
    LogicalId,
    logical_name,
    logical_picker_label,
)
from ..exports import load_mapping_json, save_mapping_header, save_mapping_json
from ..model import DeviceHello, DeviceStatus, MappingModel
from ..protocol import ClockProtocol
from ..serial_worker import SerialWorker
from ..widgets import LogPanel, MappingTable, StatusPanel


class CalibrationWindow(QtWidgets.QMainWindow):
    def __init__(self, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__(parent)
        self.setAttribute(QtCore.Qt.WidgetAttribute.WA_DeleteOnClose, False)
        self.setWindowTitle(f"{APP_NAME} - Calibration Tool")
        self.resize(1320, 880)
        self.setMinimumSize(1040, 720)

        self.model = MappingModel()
        self.device_hello = DeviceHello()
        self.device_status = DeviceStatus()
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

        splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        splitter.setChildrenCollapsible(False)
        root.addWidget(splitter, 1)

        left_container = QtWidgets.QWidget()
        left_layout = QtWidgets.QVBoxLayout(left_container)
        left_layout.setContentsMargins(0, 0, 0, 0)
        left_layout.setSpacing(8)
        left_layout.addWidget(self._build_calibration_group())
        left_layout.addWidget(self._build_mapping_tools_group())
        left_layout.addWidget(self._build_raw_group())
        left_layout.addStretch(1)

        left_scroll = QtWidgets.QScrollArea()
        left_scroll.setWidgetResizable(True)
        left_scroll.setFrameShape(QtWidgets.QFrame.Shape.NoFrame)
        left_scroll.setWidget(left_container)
        splitter.addWidget(left_scroll)

        right_splitter = QtWidgets.QSplitter(QtCore.Qt.Orientation.Vertical)
        right_splitter.setChildrenCollapsible(False)
        splitter.addWidget(right_splitter)

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

        self.log_panel = LogPanel()
        right_splitter.addWidget(self.log_panel)

        splitter.setSizes([520, 760])
        right_splitter.setSizes([210, 430, 210])
        self.statusBar().showMessage("Calibration tool ready")

    def _build_connection_bar(self) -> QtWidgets.QWidget:
        group = QtWidgets.QGroupBox("Calibration Firmware")
        layout = QtWidgets.QGridLayout(group)
        layout.setContentsMargins(10, 10, 10, 10)
        layout.setHorizontalSpacing(8)
        layout.setVerticalSpacing(8)

        self.port_combo = QtWidgets.QComboBox()
        self.port_combo.setEditable(True)
        self.port_combo.setMinimumWidth(240)
        self.refresh_ports_button = QtWidgets.QPushButton("Refresh")
        self.connect_button = QtWidgets.QPushButton("Connect")
        self.disconnect_button = QtWidgets.QPushButton("Disconnect")
        self.hello_button = QtWidgets.QPushButton("HELLO ClockCal/1")
        self.info_button = QtWidgets.QPushButton("INFO?")
        self.status_button = QtWidgets.QPushButton("STATUS?")

        note = QtWidgets.QLabel(
            "Use this window with the separate calibration/testing firmware."
        )
        note.setWordWrap(True)

        layout.addWidget(QtWidgets.QLabel("Port"), 0, 0)
        layout.addWidget(self.port_combo, 0, 1)
        layout.addWidget(self.refresh_ports_button, 0, 2)
        layout.addWidget(self.connect_button, 0, 3)
        layout.addWidget(self.disconnect_button, 0, 4)
        layout.addWidget(self.hello_button, 1, 1)
        layout.addWidget(self.info_button, 1, 2)
        layout.addWidget(self.status_button, 1, 3)
        layout.addWidget(note, 2, 0, 1, 5)

        self.refresh_ports_button.clicked.connect(
            lambda: self._queue_worker_call("refresh_ports")
        )
        self.connect_button.clicked.connect(self._connect_selected_port)
        self.disconnect_button.clicked.connect(
            lambda: self._queue_worker_call("disconnect_port")
        )
        self.hello_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.hello("ClockCal/1"))
        )
        self.info_button.clicked.connect(lambda: self._send_command(ClockProtocol.info()))
        self.status_button.clicked.connect(lambda: self._send_command(ClockProtocol.status()))
        return group

    def _build_calibration_group(self) -> QtWidgets.QGroupBox:
        group = QtWidgets.QGroupBox("Calibration Controls")
        layout = QtWidgets.QVBoxLayout(group)
        layout.setSpacing(8)

        nav = QtWidgets.QGridLayout()
        self.mode_cal_button = QtWidgets.QPushButton("MODE CAL")
        self.mode_run_button = QtWidgets.QPushButton("MODE RUN")
        self.current_button = QtWidgets.QPushButton("CUR?")
        self.map_button = QtWidgets.QPushButton("MAP?")
        self.prev_button = QtWidgets.QPushButton("Prev")
        self.next_button = QtWidgets.QPushButton("Next")
        self.set_selected_button = QtWidgets.QPushButton("Set Selected LED")
        self.validate_button = QtWidgets.QPushButton("Validate")
        self.save_device_button = QtWidgets.QPushButton("Save Device Map")
        self.load_device_button = QtWidgets.QPushButton("Load Device Map")
        self.test_segments_button = QtWidgets.QPushButton("Test Segments")
        self.test_digits_button = QtWidgets.QPushButton("Test Digits")
        self.test_all_button = QtWidgets.QPushButton("Test All")

        nav.addWidget(self.mode_cal_button, 0, 0)
        nav.addWidget(self.mode_run_button, 0, 1)
        nav.addWidget(self.current_button, 0, 2)
        nav.addWidget(self.map_button, 0, 3)
        nav.addWidget(self.prev_button, 1, 0)
        nav.addWidget(self.next_button, 1, 1)
        nav.addWidget(self.set_selected_button, 1, 2, 1, 2)
        nav.addWidget(self.validate_button, 2, 0)
        nav.addWidget(self.save_device_button, 2, 1)
        nav.addWidget(self.load_device_button, 2, 2)
        nav.addWidget(self.test_segments_button, 3, 0)
        nav.addWidget(self.test_digits_button, 3, 1)
        nav.addWidget(self.test_all_button, 3, 2)
        layout.addLayout(nav)

        picker_row = QtWidgets.QHBoxLayout()
        self.logical_picker = QtWidgets.QComboBox()
        self.logical_picker.setMinimumWidth(240)
        self.assign_current_button = QtWidgets.QPushButton("Assign To Current")
        self.assign_and_next_button = QtWidgets.QPushButton("Assign + Next")
        self.unassign_current_button = QtWidgets.QPushButton("Unassign")
        picker_row.addWidget(QtWidgets.QLabel("Logical Target"))
        picker_row.addWidget(self.logical_picker, 1)
        picker_row.addWidget(self.assign_current_button)
        picker_row.addWidget(self.assign_and_next_button)
        picker_row.addWidget(self.unassign_current_button)
        layout.addLayout(picker_row)

        self._rebuild_logical_picker()

        self.mode_cal_button.clicked.connect(lambda: self._send_command(ClockProtocol.mode_cal()))
        self.mode_run_button.clicked.connect(lambda: self._send_command(ClockProtocol.mode_run()))
        self.current_button.clicked.connect(lambda: self._send_command(ClockProtocol.current()))
        self.map_button.clicked.connect(lambda: self._send_command(ClockProtocol.mapping()))
        self.prev_button.clicked.connect(lambda: self._send_command(ClockProtocol.prev()))
        self.next_button.clicked.connect(lambda: self._send_command(ClockProtocol.next()))
        self.set_selected_button.clicked.connect(self._set_device_to_selected_row)
        self.validate_button.clicked.connect(self._validate_mapping)
        self.save_device_button.clicked.connect(lambda: self._send_command(ClockProtocol.save_map()))
        self.load_device_button.clicked.connect(lambda: self._send_command(ClockProtocol.load_map()))
        self.test_segments_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.test_segments())
        )
        self.test_digits_button.clicked.connect(
            lambda: self._send_command(ClockProtocol.test_digits())
        )
        self.test_all_button.clicked.connect(lambda: self._send_command(ClockProtocol.test_all()))
        self.assign_current_button.clicked.connect(lambda: self._assign_current(False))
        self.assign_and_next_button.clicked.connect(lambda: self._assign_current(True))
        self.unassign_current_button.clicked.connect(self._unassign_current)
        return group

    def _build_mapping_tools_group(self) -> QtWidgets.QGroupBox:
        group = QtWidgets.QGroupBox("Mapping Import / Export")
        layout = QtWidgets.QHBoxLayout(group)
        self.import_json_button = QtWidgets.QPushButton("Import JSON")
        self.export_json_button = QtWidgets.QPushButton("Export JSON")
        self.export_header_button = QtWidgets.QPushButton("Export Header")
        layout.addWidget(self.import_json_button)
        layout.addWidget(self.export_json_button)
        layout.addWidget(self.export_header_button)

        self.import_json_button.clicked.connect(self._import_json)
        self.export_json_button.clicked.connect(self._export_json)
        self.export_header_button.clicked.connect(self._export_header)
        return group

    def _build_raw_group(self) -> QtWidgets.QGroupBox:
        group = QtWidgets.QGroupBox("Raw Calibration Command")
        layout = QtWidgets.QHBoxLayout(group)
        self.raw_command_edit = QtWidgets.QLineEdit()
        self.raw_command_edit.setPlaceholderText("Send a ClockCal/1 command")
        self.raw_send_button = QtWidgets.QPushButton("Send")
        layout.addWidget(self.raw_command_edit, 1)
        layout.addWidget(self.raw_send_button)
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
            QtCore.QMetaObject.invokeMethod(self.worker, method_name, QtCore.Qt.ConnectionType.QueuedConnection)
        else:
            QtCore.QMetaObject.invokeMethod(
                self.worker,
                method_name,
                QtCore.Qt.ConnectionType.QueuedConnection,
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

    def _on_connection_changed(self, connected: bool, message: str) -> None:
        self.connected = connected
        self._append_log(message)
        self.statusBar().showMessage(message, 4000)
        if connected:
            self.current_port = self.port_combo.currentText().strip()
            self._send_commands(
                [
                    ClockProtocol.hello("ClockCal/1"),
                    ClockProtocol.info(),
                    ClockProtocol.status(),
                    ClockProtocol.current(),
                    ClockProtocol.mapping(),
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
            self.device_hello.firmware_version = parsed.data.get("FW", self.device_hello.firmware_version)
            self.device_hello.pixels = int(parsed.data.get("pixels", self.device_hello.pixels))
            self.device_hello.logical_count = int(parsed.data.get("logical", self.device_hello.logical_count))

        elif parsed.kind == "info":
            self.device_hello.board = parsed.data.get("BOARD", self.device_hello.board)
            self.device_hello.pin = parsed.data.get("PIN", self.device_hello.pin)
            self.device_hello.buttons = parsed.data.get("BTNS", self.device_hello.buttons)

        elif parsed.kind == "status":
            self.device_status.mode = parsed.data.get("MODE", self.device_status.mode)
            self.device_status.calibration_mode = True
            self.current_phys = int(parsed.data.get("current_phys", self.current_phys))

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

        elif parsed.kind == "ok":
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
        self.status_panel.set_value("protocol", self.device_hello.protocol or "ClockCal/1")
        firmware = self.device_hello.name or "Calibration firmware"
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
        self.status_panel.set_value("mode", self.device_status.mode or "CAL")
        self.status_panel.set_value("brightness", "Calibration display")
        self.status_panel.set_value("timer", "—")
        self.status_panel.set_value("stopwatch", "—")
        self.status_panel.set_value("rtc", "—")
        self.status_panel.set_value("current_phys", str(self.current_phys))
        self.status_panel.set_value("mapping", self.model.summary())

    def _refresh_connection_buttons(self) -> None:
        self.connect_button.setEnabled(not self.connected)
        self.disconnect_button.setEnabled(self.connected)
        for button in [
            self.hello_button,
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
            self.save_device_button,
            self.load_device_button,
            self.test_segments_button,
            self.test_digits_button,
            self.test_all_button,
            self.assign_current_button,
            self.assign_and_next_button,
            self.unassign_current_button,
            self.raw_send_button,
        ]:
            button.setEnabled(self.connected)

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
        else:
            self._append_log("Local mapping validation passed")
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
            target_index = self.logical_picker.findData(self.model.logical_for_phys(self.current_phys))
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
