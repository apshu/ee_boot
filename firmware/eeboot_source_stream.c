#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "eeboot.h"
#include "eeboot_source_stream.h"

static uint16_t dummy_getch(void) {
    return 0xFF00; //Irreversible error
}

static uint8_t dummy_putch(uint8_t ch) {
    return 0xFF; //Irreversible error
}

typedef enum {
    CMD_SYNC      = -1UL,
    CMD_GETCRC32  = -2UL,
} serial_stream_commands_e;

static stream_getch_func getch_func = dummy_getch;
static stream_putch_func putch_func = dummy_putch;

//Returns 0 on success, error code on fail
eeboot_ram_func uint8_t EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(uint32_t dw) {
    uint8_t retVal = 0xFF;
    if (!(retVal = putch_func(dw & 0xFF))) {
        dw >>= 8;
        if (!(retVal = putch_func(dw & 0xFF))) {
            dw >>= 8;
            if (!(retVal = putch_func(dw & 0xFF))) {
                dw >>= 8;
                retVal = putch_func(dw & 0xFF);
            }
        }
    }
    return !retVal;
}

eeboot_ram_func bool EEBOOT_SOURCE_STREAM_NAMESPACE(selectStream)(stream_getch_func getchFunction, stream_putch_func putchFunction) {
    getch_func = getchFunction ? getchFunction : dummy_getch;
    putch_func = putchFunction ? putchFunction : dummy_putch;
    if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(0UL)) {
        if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(0UL)) {
            if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(CMD_SYNC)) {
                if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(CMD_SYNC)) {
                    //Now stream synced
                    return (true);
                }
            }
        }
    }
    return (false);
}

static eeboot_ram_func bool receiveData(size_t numBytes, void* dataBuffer) {
    if (dataBuffer) {
        if (numBytes & 0xFFFF0000) {
            //Too much data
            return false;
        }
        uint8_t *databuf = (uint8_t*) dataBuffer;
        for (; numBytes; --numBytes) {
            uint16_t data_got = getch_func();
            if (data_got & 0xFF00) {
                return false;
            }
            *databuf++ = data_got;
        }
    }
    return true;
}

eeboot_ram_func bool EEBOOT_SOURCE_STREAM_NAMESPACE(seekReadData)(uint32_t offset, uint32_t numBytes, void* dataBuffer) {
    if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(CMD_SYNC)) {
        if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(CMD_SYNC)) {
            //Data stream synced
            if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(offset)) {
                if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(numBytes)) {
                    //Ready to receive data
                    return receiveData(numBytes, dataBuffer);
                }
            }
        }
    }
    return false;
}

eeboot_ram_func uint32_t EEBOOT_SOURCE_STREAM_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, uint32_t numBytes) {
    uint32_t crc32val = 0;
    if (EEBOOT_SOURCE_STREAM_NAMESPACE(seekReadData)(CMD_GETCRC32, numBytes, NULL)) {
        if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(crc32StartValue)) {
            if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(crc32Poly)) {
                if (EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(offset)) {
                // No error during data send
                    if (!receiveData(sizeof (crc32val), &crc32val)) {
                        //Problems after data receive
                        crc32val = 0;
                    }
                }
            }
        }
    }
    return crc32val;
}

