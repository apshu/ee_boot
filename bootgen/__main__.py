import enum
import functools
import json
import os
import itertools

import numexpr
from PyQt5 import QtCore, QtWidgets
from PyQt5.QtWidgets import QFileDialog, QLineEdit

from BootFormat import BootFormat
from IntelHexExtended import IntelHex
from mainscreen import Ui_MainWindow


def detect_by_bom(path, default='ascii'):
    import codecs
    with open(path, 'rb') as f:
        raw = f.read(4)  # will read less if the file is smaller
    # BOM_UTF32_LE's start is equal to BOM_UTF16_LE so need to try the former first
    for enc, boms in \
            ('utf-8-sig', (codecs.BOM_UTF8,)), \
                    ('utf-32', (codecs.BOM_UTF32_LE, codecs.BOM_UTF32_BE)), \
                    ('utf-16', (codecs.BOM_UTF16_LE, codecs.BOM_UTF16_BE)):
        if any(raw.startswith(bom) for bom in boms):
            return enc
    return default


class MainApplicationActions(QtWidgets.QMainWindow):
    @property
    def output_hex_file_name(self):
        return self.__output_hex_file_name

    @output_hex_file_name.setter
    def output_hex_file_name(self, new_value):
        self.__output_hex_file_name = new_value

    def get_project_file_io(self):
        class ProjectFileIO(enum.Enum):
            OUTPUT_HEX_NAME = (self, 'output_hex_file_name', 'output_hex_file_name')
            HEADER_VERSION = (self.ui.header_version_spin, 'value', 'setValue')
            INPUT_FILE = (self.ui.input_file_name_edt, 'text', 'setText', self.validate_file_editor)
            INPUT_FILTERS = (self.ui.input_filter_ranges_tdtedt, 'toPlainText', 'setPlainText', self.get_hex_filter_ranges)
            MCU_PROGRAM_PAGE_BYTES = (self.ui.program_page_size_edt, 'text', 'setText', self.hex_edit_field_validator)
            MCU_ERASE_PAGE_BYTES = (self.ui.erase_page_size_edt, 'text', 'setText', self.hex_edit_field_validator)
            DEVID0 = (self.ui.devid0_edt, 'text', 'setText', self.hex_edit_field_validator)
            DEVID0_ENABLED = (self.ui.devid0_used_cb, 'isChecked', 'setChecked')
            DEVID1 = (self.ui.devid1_edt, 'text', 'setText', self.hex_edit_field_validator)
            DEVID1_ENABLED = (self.ui.devid1_used_cb, 'isChecked', 'setChecked')
            DEVID2 = (self.ui.devid2_edt, 'text', 'setText', self.hex_edit_field_validator)
            DEVID2_ENABLED = (self.ui.devid2_used_cb, 'isChecked', 'setChecked')
            PCBID0 = (self.ui.pcbid0_spin, 'value', 'setValue')
            PCBID0_ENABLED = (self.ui.pcbid0_used_cb, 'isChecked', 'setChecked')
            PCBID1 = (self.ui.pcbid1_spin, 'value', 'setValue')
            PCBID1_ENABLED = (self.ui.pcbid1_used_cb, 'isChecked', 'setChecked')
            PCBID2 = (self.ui.pcbid2_spin, 'value', 'setValue')
            PCBID2_ENABLED = (self.ui.pcbid2_used_cb, 'isChecked', 'setChecked')
            FILE_COMMENT = (self.ui.file_comment_edt, 'toPlainText', 'setPlainText')
            TARGET_STORAGE = (self.ui.storage_device_cb, 'currentText', 'setCurrentText')

            def __init__(enum_self, obj=None, getter=None, setter=None, checker=None):
                noop_func = lambda *args, **kwargs: None
                getter_func = noop_func
                setter_func = noop_func
                checker_func = lambda *args, **kwargs: (None, 1)

                enum_self.obj = obj or self

                if getter:
                    if callable(getter):
                        getter_func = getter
                    else:
                        getter_func = getattr(obj, getter)
                        if not callable(getter_func):
                            getter_func = functools.partial(getattr, enum_self.obj, getter)
                if setter:
                    if callable(setter):
                        setter_func = setter
                    else:
                        setter_func = getattr(obj, setter)
                        if not callable(setter_func):
                            setter_func = functools.partial(setattr, enum_self.obj, setter)
                if checker:
                    if callable(checker):
                        checker_func = checker
                    else:
                        checker_func = getattr(obj, checker)
                        if not callable(checker_func):
                            checker_func = lambda *args, **kwargs: (None, 1)

                enum_self.__getter_func = getter_func
                enum_self.__setter_func = setter_func
                enum_self.__field_checker = checker_func

            @property
            def form_value(enum_self):
                return enum_self.__getter_func()

            @form_value.setter
            def form_value(enum_self, new_value):
                enum_self.__setter_func(new_value)

            def validate(enum_self):
                return enum_self.__field_checker(enum_self.obj)

            @property
            def form_value_asserted(enum_self):
                return_value, is_valid = enum_self.validate()
                if is_valid < 0:
                    raise AttributeError('Form value is invalid')
                return enum_self.form_value

            @classmethod
            def get_members_casefold(cls, member_name: str):
                return [member for member in cls if member.name.casefold() == member_name.casefold()]

            @classmethod
            def to_dict(cls) -> dict:
                return {element.name.casefold(): element.form_value for element in cls}

            @classmethod
            def from_dict(cls, load_from: dict):
                load_key_map = {str(key).casefold(): key for key in load_from.keys()}
                for element in cls:
                    element_name = element.name.casefold()
                    if element_name in load_key_map:
                        element.form_value = load_from[load_key_map[element_name]]

            @classmethod
            def key_set(cls):
                return {element.name.casefold() for element in cls}

            @classmethod
            def validate_all(cls):
                failed_list = []
                for field in cls:
                    field_value, validation_result = field.validate()
                    if validation_result < 0:
                        failed_list.append(field)
                return failed_list

        return ProjectFileIO

    def browse_for_input_file(self):
        _translate = QtCore.QCoreApplication.translate
        fileName = QFileDialog.getOpenFileName(parent=None, caption=_translate("OpenFile", "Open application code"), filter=_translate("OpenFile", "Intel HEX (*.hex);;All Files (*)"))
        if fileName[0]:
            ui.input_file_name_edt.setText(fileName[0])
            self.validate_file_editor(ui.input_file_name_edt)

    def clear_style_sheet(self, *args, **kwargs):
        self.sender().setStyleSheet('')

    def _append_children(self, child_or_tree):
        children = child_or_tree.children()
        if len(children):
            return itertools.chain.from_iterable(map(self._append_children, children))
        return (child_or_tree,)

    def blockSignals(self, is_blocking: bool) -> bool:
        super().blockSignals(is_blocking)
        if not hasattr(self, '__signal_blockers'):
            self.__signal_blockers = {}
        for child in self._append_children(self):
            if child not in self.__signal_blockers:
                self.__signal_blockers[child] = QtCore.QSignalBlocker(child)
            if is_blocking:
                self.__signal_blockers[child].reblock()
            else:
                self.__signal_blockers[child].unblock()

    def releaseSignals(self):
        if hasattr(self, f'_{self.__class__.__name__}__signal_blockers'):
            for blocker in self.__signal_blockers.values():
                del blocker
            del self.__signal_blockers

    def load_project_from_dict(self, load_from_dict: dict):
        load_from_dict_casefold = {str(key).casefold(): val for key, val in load_from_dict.items()}
        prjio = self.get_project_file_io()
        load_result = ''
        form_fields = prjio.key_set()
        common_keys = form_fields & load_from_dict_casefold.keys()
        if common_keys < form_fields:
            load_result = f'partial load: file missing {form_fields - common_keys} fields'

        if load_from_dict_casefold.keys() > common_keys:
            load_result += f'{"|" if load_result else ""}hint: file contains unknown extra data {load_from_dict_casefold.keys() - common_keys}'

        self.blockSignals(True)
        for form_field_name in common_keys:
            for form_field in prjio.get_members_casefold(form_field_name):
                try:
                    form_field.form_value = load_from_dict_casefold[form_field_name]
                except Exception as error:
                    load_result += f'{"|" if load_result else ""}"{form_field_name}" error {str(error)}'

        for form_field_name in common_keys:
            for form_field in prjio.get_members_casefold(form_field_name):
                form_value, validation_result = form_field.validate()
                if validation_result < 0:
                    load_result += f'{"|" if load_result else ""}"{form_field_name}" value "{form_value}" is invalid ({validation_result})'
        self.releaseSignals()

        return load_result

    def save_project_to_dict(self) -> dict:
        return self.get_project_file_io().to_dict()

    def load_project(self):
        _translate = QtCore.QCoreApplication.translate
        file_name = QFileDialog.getOpenFileName(parent=None, caption=_translate("OpenFile", "Open project"), filter=_translate("OpenFile", "BOOT project (*.bootproject);;All Files (*)"))
        if file_name[0]:
            with open(file_name[0]) as file_input:
                data = json.load(file_input)
                load_result = self.load_project_from_dict(data)
                if load_result:
                    print(f'Project file error load result: {load_result}')
                self.recalculate()

    def save_project(self):
        _translate = QtCore.QCoreApplication.translate
        file_name = QFileDialog.getSaveFileName(parent=None, caption=_translate("OpenFile", "Save project"), filter=_translate("OpenFile", "BOOT project (*.bootproject);;All Files (*)"))
        if file_name[0]:
            form_data = self.save_project_to_dict()
            if form_data:
                with open(file_name[0], 'w') as file_output:
                    json.dump(form_data, file_output, sort_keys=True, indent=4)

    def save_hex(self):
        pass

    def _form_to_eeprom_data(self):
        def val_or_default(condition_field, form_field, default = None):
            if condition_field.form_value:
                return int(str(form_field.form_value_asserted),0)
            return default

        prjio = self.get_project_file_io()
        eeprom_data = BootFormat()
        try:
            input_hex_file = IntelHex(prjio.INPUT_FILE.form_value_asserted)
            hex_ranges_list, is_valid = self.get_hex_filter_ranges()
            if is_valid < 0:
                raise AttributeError('Input file filter ranges are invalid')
            input_hex_file.apply_range_filter(hex_ranges_list)
            input_hex_file.apply_pad_to_align_page(val_or_default(prjio.MCU_ERASE_PAGE_BYTES, prjio.MCU_ERASE_PAGE_BYTES), val_or_default(prjio.MCU_PROGRAM_PAGE_BYTES,prjio.MCU_PROGRAM_PAGE_BYTES))
            eeprom_data.load_IntelHex(input_hex_file)
            eeprom_data.description = prjio.FILE_COMMENT.form_value_asserted
            eeprom_data.compatibility.DEVID0 = val_or_default(prjio.DEVID0_ENABLED, prjio.DEVID0)
            eeprom_data.compatibility.DEVID1 = val_or_default(prjio.DEVID1_ENABLED, prjio.DEVID1)
            eeprom_data.compatibility.DEVID2 = val_or_default(prjio.DEVID2_ENABLED, prjio.DEVID2)
            eeprom_data.compatibility.PCBID0 = val_or_default(prjio.PCBID0_ENABLED, prjio.PCBID0)
            eeprom_data.compatibility.PCBID1 = val_or_default(prjio.PCBID1_ENABLED, prjio.PCBID1)
            eeprom_data.compatibility.PCBID2 = val_or_default(prjio.PCBID2_ENABLED, prjio.PCBID2)
        except:
            return None
        return eeprom_data

    def recalculate(self):
        print('recalculate')
        output_data = self._form_to_eeprom_data()
        if output_data:
            IntelHex(output_data.to_dict() or {}).dump()
        # # enable save if there is something to output

    # returns source_editor content as string file name
    # can be used as a signal, in this case it handles the event in self.sender():
    # return values (absolute filepath, validation result):
    #   - 0 : editing in progress
    #   - 1 : valid input, filename in absoulte path representation
    #   - <0: Error: -1 File does not exist, -2 File not readable
    def validate_file_editor(self, source_editor=None):
        editor: QLineEdit = source_editor or self.sender()
        validation_result = 0
        border_style = ''
        edit_text = editor.text()
        if editor.hasAcceptableInput():
            if edit_text:
                edit_text = os.path.abspath(edit_text)
                if os.path.exists(edit_text):
                    if os.path.isfile(edit_text) and os.access(edit_text, os.R_OK):
                        validation_result = 1
                        border_style = '2px solid green'
                    else:
                        validation_result = -2
                        border_style = '2px dotted red'
                else:
                    # file does not exist
                    validation_result = -1
                    border_style = '3px solid red'
                editor.setText(edit_text)
        editor.setStyleSheet(f'border: {border_style}')
        return edit_text, validation_result

    # returns source_editor content as list of ranges and validity
    # can be used as a signal, in this case it handles the event in self.sender():
    # return values (list of ranges, validation result):
    #   - 0 : editing in progress
    #   - 1 : valid input, data changed to HEX representation
    #   - <0: Error: -1 invalid value, -2 invalid formula
    def get_hex_filter_ranges(self, source_editor=None):
        return_value = []
        validation_result = 0
        border_style = ''
        editor = source_editor or self.ui.input_filter_ranges_tdtedt
        editor_text = editor.toPlainText().strip()
        if editor_text:
            try:
                if not editor_text.endswith(','):
                    editor_text += ','
                ranges = eval(editor_text, {}, {})
                for from_address, to_address in ranges:
                    if from_address >= 0 and from_address.bit_length() <= 32 and to_address >= 0 and to_address.bit_length() <= 32:
                        return_value.append(range(min(from_address, to_address), max(from_address, to_address)))
                    else:
                        border_style = '3px solid red'
                        validation_result = -1
                        return_value = ()
                        break
                if return_value:
                    # fill editor with correct values
                    editor_text = ', '.join(map(lambda r: f'(0x{r.start:08X}, 0x{r.stop:08X})', return_value))
                    editor.setPlainText(editor_text)
                    # set border to validated state
                    validation_result = 1
                    border_style = '2px solid green'
            except:
                # bad formula
                validation_result = -2
                return_value = ()
                border_style = '2px dotted red'
        editor.setStyleSheet(f'border: {border_style}')
        return tuple(return_value), validation_result

    # returns source_editor content and validity
    # can be used as a signal, in this case it handles the event in self.sender():
    # return values (edit text, validation result):
    #   - 0 : editing in progress
    #   - 1 : valid input, data changed to HEX representation
    #   - <0: Error: -1 invalid value, -2 invalid formula
    def hex_edit_field_validator(self, source_editor=None):
        editor: QLineEdit = source_editor or self.sender()
        validation_result = 0
        border_style = ''
        edit_text = ''.join(editor.text().replace('_','').split())
        if editor.hasAcceptableInput():
            if edit_text:
                try:
                    eval_result = numexpr.evaluate(edit_text, global_dict={}, local_dict={}, optimization='moderate')
                    edit_val_integer: int = int(eval_result.item())
                    if edit_val_integer >= 0 and edit_val_integer.bit_length() <= 32:
                        edit_text = f'0x{edit_val_integer:08X}'
                        validation_result = 1
                        border_style = '2px solid green'
                    else:
                        validation_result = -1
                        border_style = '3px solid red'
                except Exception as e:
                    # bad formula
                    validation_result = -2
                    border_style = '2px dotted red'
        editor.setText(edit_text)
        editor.setStyleSheet(f'border: {border_style}')
        return edit_text, validation_result

    def devid0_edited(self, str):
        ui.devid0_used_cb.setChecked(bool(str.strip()))
        self.sender().setStyleSheet('')

    def devid1_edited(self, str):
        ui.devid1_used_cb.setChecked(bool(str))
        self.sender().setStyleSheet('')

    def devid2_edited(self, str):
        ui.devid2_used_cb.setChecked(bool(str))
        self.sender().setStyleSheet('')

    def pcbid0_edited(self, str):
        ui.pcbid0_used_cb.setChecked(True)

    def pcbid1_edited(self, str):
        ui.pcbid1_used_cb.setChecked(True)

    def pcbid2_edited(self, str):
        ui.pcbid2_used_cb.setChecked(True)

    def _add_programatic_ui_elements(self):
        self.__output_hex_file_name = ''

    @property
    def ui(self) -> Ui_MainWindow:
        return self.__ui

    @ui.setter
    def ui(self, new_ui):
        self.__ui = new_ui
        self._add_programatic_ui_elements()


if __name__ == '__main__':
    import sys

    app = QtWidgets.QApplication(sys.argv)
    MainWindow = MainApplicationActions()
    ui = Ui_MainWindow()
    ui.setupUi(MainWindow)
    MainWindow.ui = ui
    MainWindow.show()
    sys.exit(app.exec_())
