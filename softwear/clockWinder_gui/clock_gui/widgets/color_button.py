from __future__ import annotations

from PySide6 import QtCore, QtGui, QtWidgets


class ColorButton(QtWidgets.QPushButton):
    color_changed = QtCore.Signal(tuple)

    def __init__(self, title: str, rgb: tuple[int, int, int], parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__(parent)
        self._title = title
        self._color = QtGui.QColor(*rgb)
        self.clicked.connect(self._choose_color)
        self.setMinimumHeight(50)
        self.setSizePolicy(
            QtWidgets.QSizePolicy.Policy.Expanding,
            QtWidgets.QSizePolicy.Policy.Fixed,
        )
        self._refresh()

    def rgb(self) -> tuple[int, int, int]:
        return self._color.red(), self._color.green(), self._color.blue()

    def set_rgb(self, rgb: tuple[int, int, int], emit_signal: bool = True) -> None:
        self._color = QtGui.QColor(*rgb)
        self._refresh()
        if emit_signal:
            self.color_changed.emit(self.rgb())

    def _choose_color(self) -> None:
        color = QtWidgets.QColorDialog.getColor(
            self._color,
            self.window(),
            f"Choose {self._title}",
            QtWidgets.QColorDialog.ShowAlphaChannel,
        )
        if color.isValid():
            self._color = color
            self._refresh()
            self.color_changed.emit(self.rgb())

    def _refresh(self) -> None:
        red, green, blue = self.rgb()
        brightness = (red * 299 + green * 587 + blue * 114) / 1000
        text_color = "#161616" if brightness > 150 else "white"
        self.setText(f"{self._title}\n{red}, {green}, {blue}")
        self.setStyleSheet(
            "QPushButton {"
            f"background-color: rgb({red}, {green}, {blue});"
            f"color: {text_color};"
            "border-radius: 8px;"
            "padding: 6px 10px;"
            "font-weight: 600;"
            "text-align: center;"
            "}"
        )
