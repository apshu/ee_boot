import intelhex


class IntelHex(intelhex.IntelHex):
    def __init__(self, source=None):
        super().__init__(source)


    # Check if all data inside of allowed_range(s)
    def is_data_in_ranges(self, allowed_ranges):
        if isinstance(allowed_ranges, range):
            allowed_ranges = (allowed_ranges,)  # convert to list
        for address in self._buf.keys():
            if not any((address in allowed for allowed in allowed_ranges)):
                return False
        return True

    # Eliminate data outside of filter allowed_range(s)
    def apply_range_filter(self, allowed_ranges):
        if allowed_ranges:
            if isinstance(allowed_ranges, range):
                allowed_ranges = (allowed_ranges,)  # convert to list
            filted = {addr: data for addr, data in self._buf.items() if any((addr in allowed for allowed in allowed_ranges))}
            self._buf = filted

    def is_data_page_aligned(self, page_size):
        if page_size > 1:
            for segment_start, segment_end in self.segments():
                if segment_start % page_size or segment_end % page_size:
                    return False
        return True

    # Make data page aligned. If page size <2, nothing happens.
    # page_size is usually flash page_erase size
    # page_end is usually program row size (optimize length)
    def apply_pad_to_align_page(self, page_size, page_end = None):
        if page_end is None:
            page_end = page_size
        if page_size > 1 or page_end > 1:
            for segment_start, segment_end in self.segments():
                segment_page_start = segment_start - segment_start % page_size
                segment_page_end = segment_end + page_end - segment_end % page_end
                for address in range(segment_page_start, segment_start):
                    if address not in self._buf:
                        self._buf[address] = self.padding
                for address in range(segment_end, segment_page_end):
                    if address not in self._buf:
                        self._buf[address] = self.padding
