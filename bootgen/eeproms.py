from time import sleep

import intelhex
from pip._internal.cli.cmdoptions import progress_bar


class EEPROM_generic(object):
    CAPACITY_BYTES = 0
    WRITE_MIN_BYTES = 0
    WRITE_PAGE_BYTES = 0
    ERASE_PAGE_BYTES = 0
    BUS_NAME = ''

    def __init__(self):
        self._progress_total_bytes = -1

    @property
    def progress_total_bytes(self):
        return self._progress_total_bytes

    @progress_total_bytes.setter
    def progress_total_bytes(self, new_value):
        if self._progress_total_bytes <= 0 or new_value <= 0:
            self._progress_total_bytes = abs(new_value)
            self.progress_current_bytes = -1
            self.update_progress()
            self.progress_current_bytes = 0

    def update_progress(self):
        pass

    def reset_progress(self):
        self.progress_total_bytes = 0

    def mount(self) -> bool:
        self.reset_progress()
        return False

    def unmount(self) -> bool:
        self.reset_progress()
        return False

    def read_from_address(self, read_start_address: int, num_bytes_to_read: int) -> bytes:
        self.progress_total_bytes = num_bytes_to_read
        self.progress_current_bytes = self.progress_total_bytes
        self.update_progress()
        return None

    def _write_bytes(self, write_start_address: int, bytes_to_write) -> bool:
        self.progress_total_bytes = len(bytes_to_write)
        self.progress_current_bytes = self.progress_total_bytes
        self.update_progress()
        return False

    def _write_page(self, write_start_address: int, bytes_to_write) -> bool:
        return self._write_bytes(write_start_address=write_start_address, bytes_to_write=bytes_to_write)

    def write_and_verify_data(self, data_to_write):
        if isinstance(data_to_write, dict):
            ihex = intelhex.IntelHex(data_to_write)
            self.progress_total_bytes = len(ihex) * 2
            for segment_start, segment_end in ihex.segments():
                if not self.write_and_verify_to_address(segment_start, ihex.tobinstr(segment_start, segment_end - 1)):
                    return False
            return True
        return False

    def write_data(self, data_to_write) -> bool:
        if isinstance(data_to_write, dict):
            ihex = intelhex.IntelHex(data_to_write)
            self.progress_total_bytes = len(ihex)
            for segment_start, segment_end in ihex.segments():
                if not self.write_to_address(segment_start, ihex.tobinstr(segment_start, segment_end)):
                    break
            return True
        return False

    def write_to_address(self, write_start_address: int, bytes_to_write) -> bool:
        if bytes_to_write:
            self.progress_total_bytes = len(bytes_to_write)
            if write_start_address + len(bytes_to_write) > self.CAPACITY_BYTES:
                return False
            # page_address_mask = ((1 << self.WRITE_PAGE_BYTES.bit_length() - 1) - 1)
            write_address = write_start_address
            write_complete_address = write_start_address + len(bytes_to_write)
            page_start_address = int(write_address / self.WRITE_PAGE_BYTES) * self.WRITE_PAGE_BYTES
            data_begin = 0
            while write_address < write_complete_address:
                # find page boundaries based on current write address and PAGE size
                page_end_address = page_start_address + self.WRITE_PAGE_BYTES
                # page boundary adjusted write start and end addresses
                write_address_last_byte = min(write_complete_address, page_end_address)
                # data slicing pointers
                data_end = write_address_last_byte - write_start_address
                # sliced data
                data_chunk = bytes_to_write[data_begin: data_end]
                # execute page or byte write
                data_writer_function = self._write_bytes
                if page_start_address == write_address and page_end_address == write_address_last_byte:
                    data_writer_function = self._write_page
                if not data_writer_function(write_start_address=write_address, bytes_to_write=data_chunk):
                    return False
                # advance write pointer
                page_start_address = page_end_address
                write_address = write_address_last_byte
                data_begin = data_end
        return True

    def write_and_verify_to_address(self, write_start_address: int, bytes_to_write: bytes):
        if bytes_to_write:
            self.progress_total_bytes = len(bytes_to_write) * 2
            if self.write_to_address(write_start_address=write_start_address, bytes_to_write=bytes_to_write):
                data_written = self.read_from_address(read_start_address=write_start_address, num_bytes_to_read=len(bytes_to_write))
                return data_written == bytes_to_write
            return False
        return True


class EEP_IIC_CHUNK_READER(EEPROM_generic):
    READ_CHUNK_BYTES = 15

    def read_from_address(self, read_start_address, num_bytes_to_read):
        self.progress_total_bytes = num_bytes_to_read
        self.update_progress()
        progress_at_start = self.progress_current_bytes
        start_address = f'{read_start_address % self.CAPACITY_BYTES:05x}'
        address_L = int(start_address[3:5], 16)
        address_H = int(start_address[1:3], 16)
        address_U = int(start_address[0], 16) & 3
        self.iic_bridge.I2C_Write(self.iic_base_address | address_U, [address_H, address_L])
        data = []
        nack_ctr = 0
        while num_bytes_to_read > 0:
            data_chunk = self.iic_bridge.I2C_Read(self.iic_base_address, min(num_bytes_to_read, self.READ_CHUNK_BYTES))
            if data_chunk == -1:
                nack_ctr += 1
                if nack_ctr > 10:
                    break
                sleep(0.01)
                continue
            nack_ctr = 0
            data = data + data_chunk
            num_bytes_to_read -= len(data_chunk)
            self.progress_current_bytes += len(data)
            self.update_progress()
        self.progress_current_bytes = progress_at_start + len(data)
        self.update_progress()
        return data


class EEP_IIC_16bit_GENERIC(EEP_IIC_CHUNK_READER):
    CAPACITY_BYTES = 65536 << 3
    # WRITE_PAGE_BYTES = 256
    WRITE_PAGE_BYTES = 32  # Due to MCP2221A library limitation
    ERASE_PAGE_BYTES = 1
    WRITE_MIN_BYTES = 1
    BUS_NAME = 'IIC'

    def __init__(self, iic_bridge=None, iic_address=0x50):
        super().__init__()
        self.iic_bridge = iic_bridge
        self.__iic_base_address = iic_address
        self.datamap = {}
        self._write_bytes = self._write_bytes_device

    @property
    def iic_base_address(self):
        return self.__iic_base_address

    @property
    def test_mode(self):
        return self._write_bytes == self._write_bytes_test

    @test_mode.setter
    def test_mode(self, is_test):
        self._write_bytes = self._write_bytes_test if is_test else self._write_bytes_device
        self.read_from_address = self._read_from_address_test if is_test else super().read_from_address

    def mount(self) -> bool:
        super().mount()
        if self.test_mode:
            print('Device mounted in test mode')
            return True
        return self.iic_bridge.I2C_Read(self.iic_base_address, 1) != -1

    def unmount(self) -> bool:
        super().unmount()
        return True

    def _write_bytes_device(self, write_start_address: int, bytes_to_write) -> bool:
        self.progress_total_bytes = len(bytes_to_write)
        if write_start_address + len(bytes_to_write) > self.CAPACITY_BYTES:
            return False
        start_address = f'{write_start_address % self.CAPACITY_BYTES:05x}'
        address_L = int(start_address[3:5], 16)
        address_H = int(start_address[1:3], 16)
        address_U = int(start_address[0], 16) & 3
        self.iic_bridge.I2C_Write(self.iic_base_address | address_U, bytes([address_H, address_L]) + bytes_to_write)
        self.progress_current_bytes += len(bytes_to_write)
        self.update_progress()
        return True

    def _write_bytes_test(self, write_start_address: int, bytes_to_write: bytes) -> bool:
        print(f'write_bytes({write_start_address}, {bytes_to_write})')
        data_map = dict(zip(range(write_start_address, write_start_address + len(bytes_to_write)), bytes_to_write))
        self.datamap.update(data_map)
        self.progress_total_bytes = len(data_map)
        self.progress_current_bytes += len(data_map)
        self.update_progress()
        return True

    def _read_from_address_test(self, read_start_address, num_bytes_to_read) -> bytes:
        print(f'read_from_address({read_start_address}, {num_bytes_to_read})')
        ihex = intelhex.IntelHex(self.datamap)
        self.progress_total_bytes = num_bytes_to_read
        self.progress_current_bytes += num_bytes_to_read
        self.update_progress()
        return ihex.tobinstr(start=read_start_address, size=num_bytes_to_read)
