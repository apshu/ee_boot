from PyQt5.QtCore import pyqtSignal
from PyQt5.QtWidgets import QComboBox


class QComboBoxExtra(QComboBox):
    popup_shown:pyqtSignal = pyqtSignal()
    popup_closed:pyqtSignal = pyqtSignal()

    def showPopup(self) -> None:
        self.popup_shown.emit()
        super().showPopup()


    def hidePopup(self) -> None:
        super().hidePopup()
        self.popup_closed.emit()
