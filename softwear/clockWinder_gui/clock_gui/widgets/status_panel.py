from __future__ import annotations

from PySide6 import QtCore, QtWidgets


class StatusPanel(QtWidgets.QGroupBox):
    def __init__(self, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__("Status", parent)
        self._labels: dict[str, QtWidgets.QLabel] = {}

        form = QtWidgets.QFormLayout(self)
        form.setContentsMargins(10, 10, 10, 10)
        form.setHorizontalSpacing(12)
        form.setVerticalSpacing(6)

        for key, title in [
            ("connection", "Connection"),
            ("port", "Port"),
            ("protocol", "Protocol"),
            ("firmware", "Firmware"),
            ("board", "Board"),
            ("mode", "Mode"),
            ("brightness", "Brightness"),
            ("timer", "Timer"),
            ("stopwatch", "Stopwatch"),
            ("rtc", "RTC"),
            ("current_phys", "Current LED"),
            ("mapping", "Mapping"),
        ]:
            label = QtWidgets.QLabel("—")
            label.setTextInteractionFlags(
                label.textInteractionFlags()
                | QtCore.Qt.TextInteractionFlag.TextSelectableByMouse
            )
            label.setWordWrap(True)
            self._labels[key] = label
            form.addRow(title, label)

    def set_value(self, key: str, value: str) -> None:
        if key in self._labels:
            self._labels[key].setText(value)
