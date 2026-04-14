from __future__ import annotations

from PySide6 import QtCore, QtGui, QtWidgets

from ..constants import LOG_UNUSED, logical_name
from ..model import MappingModel


class MappingTable(QtWidgets.QTableWidget):
    physical_selected = QtCore.Signal(int)

    def __init__(self, parent: QtWidgets.QWidget | None = None) -> None:
        super().__init__(0, 4, parent)
        self.setHorizontalHeaderLabels(["Physical", "Logical", "Name", "State"])
        self.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.setSelectionMode(QtWidgets.QAbstractItemView.SingleSelection)
        self.setEditTriggers(QtWidgets.QAbstractItemView.NoEditTriggers)
        self.setAlternatingRowColors(True)
        self.setSortingEnabled(False)
        self.verticalHeader().setVisible(False)
        self.horizontalHeader().setStretchLastSection(True)
        self.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
        self.horizontalHeader().setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.horizontalHeader().setSectionResizeMode(2, QtWidgets.QHeaderView.Stretch)
        self.horizontalHeader().setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeToContents)
        self.itemSelectionChanged.connect(self._emit_selected_row)

    def set_mapping(self, model: MappingModel, current_phys: int | None) -> None:
        duplicates = model.duplicate_logicals()
        self.setRowCount(model.num_pixels)
        for phys in range(model.num_pixels):
            logical = model.logical_for_phys(phys)
            duplicate = logical in duplicates and logical != LOG_UNUSED
            state_parts = []
            if logical == LOG_UNUSED:
                state_parts.append("unassigned")
            else:
                state_parts.append("assigned")
            if duplicate:
                state_parts.append("duplicate")
            if current_phys == phys:
                state_parts.append("current")

            items = [
                QtWidgets.QTableWidgetItem(str(phys)),
                QtWidgets.QTableWidgetItem("UNUSED" if logical == LOG_UNUSED else str(logical)),
                QtWidgets.QTableWidgetItem(logical_name(logical)),
                QtWidgets.QTableWidgetItem(", ".join(state_parts)),
            ]
            for column, item in enumerate(items):
                item.setTextAlignment(
                    QtCore.Qt.AlignVCenter
                    | (QtCore.Qt.AlignRight if column in (0, 1) else QtCore.Qt.AlignLeft)
                )
                if duplicate:
                    item.setBackground(QtGui.QColor("#472626"))
                    item.setForeground(QtGui.QBrush(QtGui.QColor("#ffd8d8")))
                elif current_phys == phys:
                    item.setBackground(QtGui.QColor("#1f3f5b"))
                    item.setForeground(QtGui.QBrush(QtGui.QColor("white")))
                self.setItem(phys, column, item)

        if current_phys is not None and 0 <= current_phys < model.num_pixels:
            self.selectRow(current_phys)

    def selected_physical(self) -> int | None:
        indexes = self.selectionModel().selectedRows()
        if not indexes:
            return None
        return indexes[0].row()

    def _emit_selected_row(self) -> None:
        selected = self.selected_physical()
        if selected is not None:
            self.physical_selected.emit(selected)
