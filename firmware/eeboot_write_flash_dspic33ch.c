#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "eeboot.h"
#include "eeboot_write_flash_dspic33ch.h"

#define WRITE_PAGE_BYTES 64*(3+1)
#define ERASE_PAGE_BYTES 512*(3+1)

/* Return nonzero if error */
eeboot_ram_func static uint32_t NVMUnlock(uint32_t nvmop) {
    return 0;
    // Flash operation to perform
    NVMCON = nvmop;
    // Suspend all Interrupts
    __builtin_disi(5);
    // Enable Flash Write/Erase Operations and Select
    // Write Keys
    // Start the operation using the Set Register
    __builtin_write_NVM();
    // Wait for operation to complete
    while (NVMCON & _NVMCON_WR_MASK) continue;
    // Disable NVM write enable
    NVMCONbits.WREN = 0;
    // Return WRERR and LVDERR Error Status Bits
    return (NVMCON & (_NVMCON_WRERR_MASK | _NVMCON_URERR_MASK));
}

/* Erase pages */
eeboot_ram_func static bool erase_page(uint32_t erase_address) {
    NVMADR = erase_address & 0xFFFF;
    NVMADRU = (erase_address >> 16)& 0xFFFF;
    // Unlock and Erase Page
    return !NVMUnlock(_NVMCON_WREN_MASK | 3);
}

/* Write a full page */
eeboot_ram_func static bool write_page(uint32_t write_address, void *data) {
    //At this moment only uncompressed format supported
    NVMCONbits.RPDF = 0;
    NVMADRU = (write_address >> 16)& 0xFFFF;
    NVMADR = write_address & 0xFFFF;
    uint32_t data_loc = (ptrdiff_t)data;
    NVMSRCADRL = data_loc & 0xFFFF; //Need physical memory location
    NVMSRCADRH = (data_loc >> 16) & 0xFF; //Need physical memory location
    return !NVMUnlock(_NVMCON_WREN_MASK | 2);
}

/* Write 4 or 8 bytes */
eeboot_ram_func static bool write_dorqword(uint32_t write_address, void *data, bool isQ) {
    if (isQ) {
        TBLPAG = 0xFA; //From datasheet
        NVMADRU = (write_address >> 16)& 0xFFFF;
        NVMADR = write_address & 0xFFFF;
        uint16_t *word_data = (uint16_t*)data;
        __builtin_tblwtl(0,*word_data++); // load write latches
        __builtin_tblwth(0,*word_data++);
        __builtin_tblwtl(0x2,*word_data++);
        __builtin_tblwth(0x2,*word_data++);
        return !NVMUnlock(_NVMCON_WREN_MASK | 1);
    }
    return false;
}

static uint32_t intFlashSourceBuffer[WRITE_PAGE_BYTES / 4]; // Coherent variable for FLASH data

eeboot_ram_func bool EEBOOT_WRITE_INTFLASH_NAMESPACE(storeDataSegment)(eeboot_segmentDescriptor_t *dataSegment) {
    uint32_t bytesToWrite = dataSegment->length;
    uint32_t readPosition = dataSegment->sourceAddress;
    uint32_t writeAddress = dataSegment->destinationAddress;
    //CRC32 already verified
    //Verify alignment
    if ((bytesToWrite & (WRITE_PAGE_BYTES - 1)) || (writeAddress & (ERASE_PAGE_BYTES - 1))) {
        //Misaligned data request
        if (bytesToWrite == 8) {
            //Special request to write vectors or CRC as last write. Only if input is formatted so
            if (eeboot_seekReadData(readPosition, bytesToWrite, intFlashSourceBuffer)) {
                write_dorqword(writeAddress, intFlashSourceBuffer, true);
            }
        }
        return false;
    }
    for (; bytesToWrite; bytesToWrite -= WRITE_PAGE_BYTES, writeAddress += WRITE_PAGE_BYTES, readPosition += WRITE_PAGE_BYTES) {
        if (!eeboot_seekReadData(readPosition, WRITE_PAGE_BYTES, intFlashSourceBuffer)) {
            return false;
        }
        if ((writeAddress & (ERASE_PAGE_BYTES - 1)) == 0) {
            if (!erase_page(writeAddress)) {
                return false;
            }
        }
        if (!write_page(writeAddress, intFlashSourceBuffer)) {
            return false;
        }
        uint32_t *verifyBuffer = &writeAddress;
        int counter = WRITE_PAGE_BYTES / 4;
        while (counter--) {
            if (verifyBuffer[counter] != intFlashSourceBuffer[counter]) {
                return false;
            }
        }
    }
    return true;
}