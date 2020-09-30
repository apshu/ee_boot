import functools
import itertools

import hid
from PyMCP2221A import PyMCP2221A
from PyQt5 import QtWidgets
from PyQt5.QtCore import QTimerEvent, QThread, pyqtSignal, pyqtSlot, QRunnable, QThreadPool, QObject
from PyQt5.QtWidgets import QDialog

import eeproms
import textwrap

from data2eep import EEPROM_Programmer
from programming_ui import Ui_ProgrammerDialog

class MCP2221A(PyMCP2221A.PyMCP2221A):
    def __init__(self, hid_path):
        self.CLKDUTY_0 = 0x00
        self.CLKDUTY_25 = 0x08
        self.CLKDUTY_50 = 0x10
        self.CLKDUTY_75 = 0x18
        # self.CLKDIV_1 = 0x00    # 48MHz  Dont work.
        self.CLKDIV_2 = 0x01  # 24MHz
        self.CLKDIV_4 = 0x02  # 12MHz
        self.CLKDIV_8 = 0x03  # 6MHz
        self.CLKDIV_16 = 0x04  # 3MHz
        self.CLKDIV_32 = 0x05  # 1.5MHz
        self.CLKDIV_64 = 0x06  # 750KHz
        self.CLKDIV_128 = 0x07  # 375KHz
        self.mcp2221a = hid.device()
        self.mcp2221a.open_path(hid_path)


class ProgrammerThread(EEPROM_Programmer, QThread):
    update = pyqtSignal(int, name='changed')
    finished = pyqtSignal(name='finished')
    init_progress = pyqtSignal(int, name='init_progress')
    error = pyqtSignal(str, name='error')

    def __init__(self, memory_adapter=None, data={}):
        QThread.__init__(self)
        EEPROM_Programmer.__init__(self, memory_adapter=memory_adapter, data=data)
        self.__terminate = False

    def update_progress(self):
        if self._memory.progress_current_bytes < 0:
            self.init_progress.emit(self._memory.progress_total_bytes)
        self.update.emit(self._memory.progress_current_bytes)
        print(f'{self._memory.progress_current_bytes}/{self._memory.progress_total_bytes}')
        self.msleep(100)

    def operation_error(self, error_message):
        self.error.emit(error_message)

    def operation_finished(self):
        self.finished.emit()

    @pyqtSlot()
    def run(self):
        super(ProgrammerThread, self).run()

class ProgrammerApplicationWindow(QDialog):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__hid_timer_id = 0
        self.__data = {}

    def show(self, default_mcp=None) -> None:
        super().show()
        self.populate_programmers()

    @pyqtSlot()
    def populate_programmers(self, programmer_list=None, default_mcp=None):
        if self.__hid_timer_id:
            self.killTimer(self.__hid_timer_id)
            self.__hid_timer_id = 0
        self.ui.program_progress.setMaximum(1)
        self.ui.program_progress.setMinimum(0)
        self.ui.program_progress.setValue(0)
        programmer_list = programmer_list or hid.enumerate(0x04D8, 0x00DD)
        programmer_paths = tuple(prog['path'] for prog in programmer_list)
        old_paths = []
        item_id = 0
        while item_id < self.ui.programmer_cb.count():
            item_path = self.ui.programmer_cb.itemData(item_id)
            if item_path in programmer_paths:
                old_paths.append(item_path)
                item_id += 1
            else:
                self.ui.programmer_cb.removeItem(item_id)
                print('Removed old')
        for prog in programmer_list:
            if prog['path'] not in old_paths:
                print('Add new')
                self.ui.programmer_cb.addItem(f'{prog["product_string"]}{"" if not prog["serial_number"] else " (" + prog["serial_number"] + ")"}', prog['path'])
        self.ui.programmer_cb.setEnabled(self.ui.programmer_cb.count() > 1)
        self.ui.program_btn.setEnabled(self.ui.programmer_cb.count() > 0)
        if default_mcp:
            self.ui.programmer_cb.currentIndex = -1
            for item_id in range(self.ui.programmer_cb.count()):
                if self.ui.programmer_cb.itemData(item_id) == default_mcp:
                    self.ui.programmer_cb.currentIndex = item_id
                    break
        self.__hid_timer_id = self.startTimer(2000)

    def timerEvent(self, event: QTimerEvent) -> None:
        if event.timerId() == self.__hid_timer_id:
            self.killTimer(self.__hid_timer_id)
            self.__hid_timer_id = 0
        self.populate_programmers()

    @staticmethod
    def prepare_programming(iic_bridge_path, data_set_dict):
        iic_bridge = MCP2221A(iic_bridge_path)
        memory_device = eeproms.EEP_IIC_16bit_GENERIC(iic_bridge=iic_bridge)
        # memory_device.test_mode = True
        return ProgrammerThread(memory_adapter=memory_device, data=data_set_dict)

    def start_programming(self):
        if self.__hid_timer_id:
            self.killTimer(self.__hid_timer_id)
            self.__hid_timer_id = 0
        self.programmer_thread = self.prepare_programming(self.ui.programmer_cb.currentData(), self.data_set)
        self.programmer_thread.update.connect(self.ui.program_progress.setValue)
        self.programmer_thread.finished.connect(self.populate_programmers)
        self.programmer_thread.init_progress.connect(self.ui.program_progress.setMaximum)
        self.ui.program_btn.setEnabled(False)
        self.programmer_thread.start()

    @property
    def data_set(self):
        return self.__data

    @data_set.setter
    def data_set(self, data_dict: dict):
        self.__data = data_dict

    @property
    def ui(self) -> Ui_ProgrammerDialog:
        return self.__ui

    @ui.setter
    def ui(self, new_ui):
        self.__ui = new_ui


if __name__ == '__main__':
    import sys

    app = QtWidgets.QApplication(sys.argv)
    print(app.arguments())
    MainWindow = ProgrammerApplicationWindow()
    MainWindow.data_set = dict(zip(range(1400), itertools.cycle(range(10,100))))
    programmer_ui = Ui_ProgrammerDialog()
    programmer_ui.setupUi(MainWindow)
    MainWindow.ui = programmer_ui
    MainWindow.show()
    sys.exit(app.exec_())
