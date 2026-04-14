from __future__ import annotations

from PySide6 import QtWidgets


class LogPanel(QtWidgets.QGroupBox):
    def __init__(self, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__("Log", parent)
        layout = QtWidgets.QVBoxLayout(self)
        layout.setContentsMargins(8, 8, 8, 8)
        self.text_edit = QtWidgets.QPlainTextEdit()
        self.text_edit.setReadOnly(True)
        self.text_edit.setMaximumBlockCount(1200)
        self.text_edit.setMinimumHeight(170)
        clear_button = QtWidgets.QPushButton("Clear Log")
        clear_button.clicked.connect(self.text_edit.clear)
        layout.addWidget(self.text_edit, 1)
        layout.addWidget(clear_button)

    def append(self, line: str) -> None:
        self.text_edit.appendPlainText(line)
        scrollbar = self.text_edit.verticalScrollBar()
        scrollbar.setValue(scrollbar.maximum())
