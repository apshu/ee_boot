#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "eeboot.h"
#include "eeboot_source_intflash.h"

eeboot_ram_func bool EEBOOT_SOURCE_INTFLASH_NAMESPACE(seekReadData)(uint32_t offset, size_t numBytes, void* dataBuffer) {
    while (numBytes) {
        --numBytes;
        ((uint8_t*) dataBuffer)[numBytes] = *(uint8_t*)(offset + numBytes);
    }
    return true;
}

eeboot_ram_func uint32_t EEBOOT_SOURCE_INTFLASH_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, size_t numBytes) {
    int k;
    uint8_t *buf = (uint8_t*)offset;
//    crc32StartValue = ~crc32StartValue;
    while (numBytes--) {
        crc32StartValue ^= *buf++;
        for (k = 0; k < 8; k++)
            crc32StartValue = (crc32StartValue & 1) ? ((crc32StartValue >> 1) ^ crc32Poly) : (crc32StartValue >> 1);
    }
    return crc32StartValue;
//    return ~crc32StartValue;
}