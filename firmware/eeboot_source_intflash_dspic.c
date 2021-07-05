#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <libpic30.h>
#include "eeboot.h"
#include "eeboot_source_intflash.h"

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

eeboot_ram_func bool EEBOOT_SOURCE_INTFLASH_NAMESPACE(seekReadData)(uint32_t offset, uint32_t numBytes, void* dataBuffer) {
    (void) _memcpy_p2d24(dataBuffer, offset, numBytes);
    return true;
}

eeboot_ram_func uint32_t EEBOOT_SOURCE_INTFLASH_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, uint32_t numBytes) {
    int k;
    uint8_t buf[16];
    int bufptr = 0;
    while (numBytes > 0) {
        if (!(bufptr % sizeof(buf))) {
            //Buffer exhausted
            int readsize = min(sizeof(buf), numBytes);
            offset = _memcpy_p2d24(buf, offset, readsize);
            numBytes -= readsize;
            bufptr = 0;
        }
        crc32StartValue ^= buf[bufptr++];
        for (k = 0; k < 8; k++)
            crc32StartValue = (crc32StartValue & 1) ? ((crc32StartValue >> 1) ^ crc32Poly) : (crc32StartValue >> 1);
    }
    return crc32StartValue;
}