import codecs
import struct
import typing

import crcmod
from intelhex import IntelHex

from USBEEFormat import USBEEFormat


def isInt_float(s):
    try:
        return float(str(s)).is_integer()
    except:
        return False


def to_int(s, default=None):
    if not (default is None or isInt_float(s)):
        return default
    return int(float(str(s)))


class _boot_section():
    def __init__(self):
        self._DATA_FORMAT = '<3I4s'
        self._data_bytes = b''
        self._destination_address = self._source_address = 0xFFFFFFFF  # unused record

    @property
    def _len_data_bytes(self):
        return len(self._data_bytes) or 0xFFFFFFFF

    @property
    def num_data_bytes(self):
        return len(self.data_bytes)

    @property
    def len_descriptor(self):
        return struct.calcsize(self._DATA_FORMAT)

    @property
    def source_address(self):
        return self._source_address if self._data_bytes else 0xFFFFFFFF

    @source_address.setter
    def source_address(self, address):
        self._source_address = to_int(address)

    @property
    def destination_address(self):
        return self._destination_address if self._data_bytes else 0xFFFFFFFF

    @destination_address.setter
    def destination_address(self, address):
        self._destination_address = to_int(address)

    @property
    def data_bytes(self):
        return self._data_bytes or b''

    @data_bytes.setter
    def data_bytes(self, data_bytes):
        self._data_bytes = data_bytes

    @property
    def data_CRC32(self) -> bytes:
        if self._data_bytes:
            crcCalc = crcmod.Crc(poly=0x104C11DB7, initCrc=0x544f4f42, xorOut=0)
            crcCalc.update(self._data_bytes)
            return crcCalc.digest()
        return b'\xFF' * 4  # End of record

    def header_bytes(self) -> bytes:
        outbytes = struct.pack(self._DATA_FORMAT, self.destination_address, self.source_address, self._len_data_bytes, self.data_CRC32)
        return outbytes

    def inject_to_intel_hex(self, ihex: IntelHex, header_address: int) -> bool:  # returns false if this is the is the last record
        the_header = self.header_bytes()
        ihex.frombytes(the_header, offset=header_address)
        if self._data_bytes:
            ihex.frombytes(self._data_bytes, offset=self.source_address)
            return True
        return False

    def __repr__(self):
        if self._data_bytes:
            return f'<_boot_section: {self.len_data_bytes} bytes at dest={hex(self.destination_address)} from src={hex(self.source_address)}>'
        return f'<_boot_section: END MARKER>'


class _compatibility(dict):

    def __init__(self):
        super().__init__({'DEVID0': None, 'DEVID1': None, 'DEVID1': None, 'PCBID0': None, 'PCBID1': None, 'PCBID2': None, })
        self.__VERSION = False
        self.__is_changed = False

    def __getattr__(self, name):
        if name in self:
            return self[name]
        return object.__getattr__(name)

    def __setattr__(self, name, value):
        if name in self:
            self.__is_changed = self.__is_changed or self[name] != value
            self[name] = value
        else:
            object.__setattr__(self, name, value)

    @property
    def is_changed(self):
        return self.__is_changed

    @property
    def VERSION(self):
        return self.__VERSION & 3

    @VERSION.setter
    def VERSION(self, ver):
        self.__VERSION = ver & 3

    def _getid(self, idname):
        if hasattr(self, idname):
            idvalue = getattr(self, idname)
            if idvalue is not None:
                idvalue = int(str(idvalue), 0)
                return 1, struct.pack('<I', idvalue)
        return 0, b'\0\0\0\0'

    def _addid(self, data_bytes, name, length=4):
        is_present, the_bytes = self._getid(name)
        data_bytes[0] <<= 1
        data_bytes[0] |= is_present
        data_bytes[1:1] = list(the_bytes[:length])

    def to_bytes(self) -> bytes:
        self.__is_changed = False
        data_bytes = [0]
        self._addid(data_bytes, 'PCBID2', 1)
        self._addid(data_bytes, 'DEVID2', 4)
        self._addid(data_bytes, 'PCBID1', 1)
        self._addid(data_bytes, 'DEVID1', 4)
        self._addid(data_bytes, 'PCBID0', 1)
        self._addid(data_bytes, 'DEVID0', 4)
        data_bytes[0] |= self.VERSION << 6
        return bytes(data_bytes)


class BootFormat(USBEEFormat):
    def __init__(self, loadfrom=None):
        self._cache = {}
        self._ihex = loadfrom
        self._segments: typing.List[_boot_section] = []
        self.__description = ''
        self.compatibility = _compatibility()

    @property
    def description(self):
        return self.__description

    @description.setter
    def description(self, value):
        if self.__description != value:
            self._cache = {}
            self.__description = value

    def to_dict(self):
        if (not self._cache) or self.compatibility.is_changed:
            output = IntelHex()
            encoded_description = b''
            try:
                encoded_description = self.description.encode('ascii')
            except:
                try:
                    encoded_description = codecs.BOM_UTF8 + self.description.encode('utf-8')
                except:
                    try:
                        encoded_description = codecs.BOM_UTF16_BE + self.description.encode('utf-16-be')
                    except:
                        try:
                            encoded_description = codecs.BOM_UTF32_BE + self.description.encode('utf-32-be')
                        except:
                            raise UnicodeEncodeError('Cannot encode text in ASCII or UTF-8 or UTF-16 or UTF-32')
            header_total_length = 28 + len(self._segments) * _boot_section().len_descriptor + len(encoded_description)
            binary_data_pointer = header_total_length
            header_total_length_bytes = struct.pack('<I', header_total_length)
            output.frombytes(header_total_length_bytes + b'BOOT' + self.compatibility.to_bytes(), 4)
            segment_descriptor_address = 28
            for segment in self._segments:
                segment.source_address = binary_data_pointer
                segment.inject_to_intel_hex(output, segment_descriptor_address)
                segment_descriptor_address += segment.len_descriptor
                binary_data_pointer += segment.num_data_bytes
            if self.description:
                output.frombytes(encoded_description, segment_descriptor_address)
            crcCalc = crcmod.Crc(poly=0x104C11DB7, initCrc=0x544f4f42, xorOut=0)
            crcCalc.update(output.tobinstr(4, header_total_length - 1))
            crc32 = crcCalc.digest()
            output.frombytes(crc32)
            self._cache = output.todict()
        return self._cache

    # Returns the length of the data bytes (excl. header)
    def __len__(self):
        ln = sum(map(lambda item: item.num_data_bytes, self._segments), 0)
        return ln

    def load_IntelHex(self, ihex: IntelHex):
        self._segments = []
        self._cache = {}
        new_segments = [_boot_section()]
        for data_start, data_end in ihex.segments():
            current_segment = _boot_section()
            current_segment.destination_address = data_start
            current_segment.data_bytes = ihex.tobinstr(data_start, data_end - 1)
            new_segments.append(current_segment)
        if new_segments:
            self._segments = sorted(new_segments, key=lambda data_seg: data_seg.destination_address)
