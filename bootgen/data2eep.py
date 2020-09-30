import eeproms


class EEPROM_Programmer(object):

    def __init__(self, memory_adapter=None, data={}):
        self._memory: eeproms.EEPROM_generic = memory_adapter
        self._data_dict = data

    def update_progress(self):
        if self._memory.progress_current_bytes > 0:
            print('.')

    def operation_finished(self):
        print(f'{self._memory.progress_current_bytes}/{self._memory.progress_total_bytes} bytes written and verified')

    def operation_error(self, error_message):
        print(f'ERROR:{error_message} at stage {self._memory.progress_current_bytes}/{self._memory.progress_total_bytes}')

    def run(self):
        if self._memory and self._data_dict:
            try:
                self._memory.update_progress = self.update_progress
                self._memory.reset_progress()
                if not self._memory.write_and_verify_data(self._data_dict):
                    self.operation_error('Data verify failed')
            except Exception as e:
                self.operation_error(f'Error during writing:{str(e)}')
        else:
            self.operation_error('Nothing to write')
        self.operation_finished()
