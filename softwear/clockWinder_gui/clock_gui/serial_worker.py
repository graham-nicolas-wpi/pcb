from __future__ import annotations

from typing import Optional

import serial
from PySide6 import QtCore
from serial.tools import list_ports

from .constants import SERIAL_BAUDRATE, SERIAL_POLL_INTERVAL_MS


class SerialWorker(QtCore.QObject):
    ports_changed = QtCore.Signal(list)
    connection_changed = QtCore.Signal(bool, str)
    line_received = QtCore.Signal(str)
    worker_log = QtCore.Signal(str)

    def __init__(self) -> None:
        super().__init__()
        self._serial: Optional[serial.Serial] = None
        self._poll_timer: Optional[QtCore.QTimer] = None
        self._buffer = bytearray()
        self._port_name = ""

    @QtCore.Slot()
    def start(self) -> None:
        if self._poll_timer is not None:
            return
        self._poll_timer = QtCore.QTimer(self)
        self._poll_timer.setInterval(SERIAL_POLL_INTERVAL_MS)
        self._poll_timer.timeout.connect(self._poll)
        self._poll_timer.start()

    @QtCore.Slot()
    def refresh_ports(self) -> None:
        ports = sorted(port.device for port in list_ports.comports())
        self.ports_changed.emit(ports)

    @QtCore.Slot(str)
    def connect_port(self, port_name: str) -> None:
        self.disconnect_port()
        clean_port = port_name.strip()
        if not clean_port:
            self.connection_changed.emit(False, "No serial port selected")
            return

        try:
            serial_port = serial.Serial(
                port=clean_port,
                baudrate=SERIAL_BAUDRATE,
                timeout=0,
                write_timeout=0,
                rtscts=False,
                dsrdtr=False,
                xonxoff=False,
            )
            self._serial = serial_port
            self._port_name = clean_port
            self._buffer.clear()
            try:
                self._serial.reset_input_buffer()
                self._serial.reset_output_buffer()
            except Exception:
                pass
            self.connection_changed.emit(True, f"Connected to {clean_port} @ {SERIAL_BAUDRATE}")
        except Exception as exc:
            self._serial = None
            self._port_name = ""
            self.connection_changed.emit(False, f"Connect failed: {exc}")

    @QtCore.Slot()
    def disconnect_port(self) -> None:
        if self._serial is None:
            return
        port_name = self._port_name or self._serial.port
        try:
            self._serial.close()
        except Exception:
            pass
        self._serial = None
        self._port_name = ""
        self._buffer.clear()
        self.connection_changed.emit(False, f"Disconnected from {port_name}")

    @QtCore.Slot(str)
    def send_line(self, line: str) -> None:
        if self._serial is None:
            self.worker_log.emit("Not connected; command was not sent")
            return
        payload = line.strip()
        if not payload:
            return
        try:
            self._serial.write((payload + "\n").encode("utf-8"))
        except Exception as exc:
            self._handle_disconnect(f"Write failed: {exc}")

    def _poll(self) -> None:
        if self._serial is None:
            return
        try:
            waiting = self._serial.in_waiting
            if waiting <= 0:
                return
            chunk = self._serial.read(waiting)
            if not chunk:
                return
            self._buffer.extend(chunk)
            while b"\n" in self._buffer:
                raw_line, _, remainder = self._buffer.partition(b"\n")
                self._buffer = bytearray(remainder)
                line = raw_line.decode("utf-8", errors="replace").strip()
                if line:
                    self.line_received.emit(line)
        except Exception as exc:
            self._handle_disconnect(f"Serial error: {exc}")

    def _handle_disconnect(self, message: str) -> None:
        if self._serial is not None:
            try:
                self._serial.close()
            except Exception:
                pass
        self._serial = None
        self._buffer.clear()
        self.connection_changed.emit(False, message)
