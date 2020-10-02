import crcmod


counter = 1

def reverse_mask(x):
    x = ((x & 0x55555555) << 1) | ((x & 0xAAAAAAAA) >> 1)
    x = ((x & 0x33333333) << 2) | ((x & 0xCCCCCCCC) >> 2)
    x = ((x & 0x0F0F0F0F) << 4) | ((x & 0xF0F0F0F0) >> 4)
    x = ((x & 0x00FF00FF) << 8) | ((x & 0xFF00FF00) >> 8)
    x = ((x & 0x0000FFFF) << 16) | ((x & 0xFFFF0000) >> 16)
    return x


def showcrc(poly, start, reverse, xorval):
    start = start & 0xffffffff
    xorval = xorval & 0xffffffff
    reverse = bool(reverse)
    global counter
    crc_func = crcmod.mkCrcFun(poly, start, reverse, xorval)
    crc_res = crc_func(bytes(range(16)))
    print(f'{counter:2}) 0x{crc_res:08X}     \N{RIGHTWARDS ARROW}    crcmod.mkCrcFun(poly=0x{poly:08X}, initCrc=0x{start:08X}, rev={reverse}, xorOut=0x{xorval:08X})', end='')
    if crc_res == lookfor:
        print(f'  \N{check mark}')
    else:
        print()
    counter += 1


def iterate(poly, startval = 0, xorval = 0):
    showcrc(poly, startval, False, xorval)
    showcrc(poly, startval, False, ~xorval)
    showcrc(poly, startval, True, xorval)
    showcrc(poly, startval, True, ~xorval)
    showcrc(poly, ~startval, False, xorval)
    showcrc(poly, ~startval, False, ~xorval)
    showcrc(poly, ~startval, True, xorval)
    showcrc(poly, ~startval, True, ~xorval)

def iterate_both(poly, startval = 0, xorval = 0):
    print(f'Polygon: 0x{POLY:08X}')
    iterate(0x100000000 | POLY, startval, xorval)
    print(f'Refelcted polygon: 0x{reverse_mask(POLY):08X}')
    iterate(reverse_mask(POLY) | 0x100000000, startval, xorval)

POLY = 0x04C11DB7
lookfor = 0xCBC6DC44
print(f'Looking for result: {lookfor} for data 0...15: {list(range(16))}')
iterate_both(POLY, startval = 0x544F4F42)
