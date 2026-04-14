from __future__ import annotations

import sys

from PySide6 import QtGui, QtWidgets

from .constants import APP_NAME
from .windows.main_window import MainWindow


def _apply_style(app: QtWidgets.QApplication) -> None:
    app.setStyle("Fusion")
    app.setApplicationDisplayName(APP_NAME)
    app.setOrganizationName("pcb")
    app.setApplicationName("clock_gui")

    palette = QtGui.QPalette()
    palette.setColor(QtGui.QPalette.Window, QtGui.QColor("#10161b"))
    palette.setColor(QtGui.QPalette.WindowText, QtGui.QColor("#ebeff2"))
    palette.setColor(QtGui.QPalette.Base, QtGui.QColor("#162028"))
    palette.setColor(QtGui.QPalette.AlternateBase, QtGui.QColor("#1b2831"))
    palette.setColor(QtGui.QPalette.ToolTipBase, QtGui.QColor("#162028"))
    palette.setColor(QtGui.QPalette.ToolTipText, QtGui.QColor("#ebeff2"))
    palette.setColor(QtGui.QPalette.Text, QtGui.QColor("#ebeff2"))
    palette.setColor(QtGui.QPalette.Button, QtGui.QColor("#22313c"))
    palette.setColor(QtGui.QPalette.ButtonText, QtGui.QColor("#ebeff2"))
    palette.setColor(QtGui.QPalette.Highlight, QtGui.QColor("#2d82b7"))
    palette.setColor(QtGui.QPalette.HighlightedText, QtGui.QColor("white"))
    app.setPalette(palette)

    app.setStyleSheet(
        """
        QWidget {
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #314654;
            border-radius: 10px;
            margin-top: 12px;
            padding-top: 12px;
            background: #131d24;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 12px;
            padding: 0 4px;
            color: #d8e6ef;
            font-weight: 700;
        }
        QPushButton {
            background: #2a4f68;
            border: 1px solid #467896;
            border-radius: 8px;
            padding: 6px 12px;
        }
        QPushButton:hover {
            background: #346786;
        }
        QPushButton:pressed {
            background: #234b63;
        }
        QLineEdit, QComboBox, QSpinBox, QDateTimeEdit, QPlainTextEdit, QTableWidget {
            background: #162028;
            color: #ebeff2;
            border: 1px solid #314654;
            border-radius: 8px;
            padding: 6px;
        }
        QLineEdit::placeholder {
            color: #8fa4b2;
        }
        QAbstractSpinBox {
            color: #ebeff2;
        }
        QComboBox QAbstractItemView {
            color: #ebeff2;
            background: #162028;
            selection-background-color: #2d82b7;
            selection-color: white;
        }
        QHeaderView::section {
            background: #22313c;
            padding: 6px;
            border: 0;
            border-right: 1px solid #314654;
            border-bottom: 1px solid #314654;
            font-weight: 600;
        }
        QScrollArea {
            border: none;
        }
        """
    )


def main() -> int:
    app = QtWidgets.QApplication(sys.argv)
    _apply_style(app)
    window = MainWindow()
    window.show()
    return app.exec()
