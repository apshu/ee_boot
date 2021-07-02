#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "eeboot.h"
#include "eeboot_write_flash_pic32m.h"

#define WRITE_PAGE_BYTES 64*4
#define ERASE_PAGE_BYTES 512*4

/* Return nonzero if error */
eeboot_ram_func static uint32_t NVMUnlock(uint32_t nvmop) {
    uint32_t status;
    // Suspend or Disable all Interrupts
    asm volatile (" di %0 " : "=r" (status));
    // Enable Flash Write/Erase Operations and Select
    // Flash operation to perform
    NVMCON = nvmop;
    // Write Keys
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    // Start the operation using the Set Register
    NVMCONSET = 0x8000;
    // Wait for operation to complete
    while ((NVMCON & 0x8000) != 0) { continue; }
    // Restore Interrupts
    if (status & 0x00000001) {
        asm volatile (" ei");
    } else {
        asm volatile (" di");
    }
    // Disable NVM write enable
    NVMCONCLR = 0x0004000;
    // Return WRERR and LVDERR Error Status Bits
    return (NVMCON & 0x3000);
}

/* Erase pages */
eeboot_ram_func static bool erase_page(uint32_t erase_address) {
    NVMADDR = erase_address;
    // Unlock and Erase Page
    return !NVMUnlock(0x4004);
}

/* Write a full page */
eeboot_ram_func static bool write_page(uint32_t write_address, void *data) {
    NVMADDR = write_address;
    NVMSRCADDR = ((uint32_t) data) & 0x1FFFFFFF; //Need physical memory location
    return !NVMUnlock(0x4003);
}

/* Write 4 or 8 bytes */
eeboot_ram_func static bool write_dorqword(uint32_t write_address, void *data, bool isQ) {
    NVMADDR = write_address;
    NVMDATA0 = *(uint32_t*) data;
    NVMDATA1 = ((uint32_t*) data)[1];
    return !NVMUnlock(isQ ? 0x4010 : 0x4001);
}

static uint32_t __attribute__((coherent)) intFlashSourceBuffer[WRITE_PAGE_BYTES / 4]; // Coherent variable for FLASH data

eeboot_ram_func bool EEBOOT_WRITE_INTFLASH_NAMESPACE(storeDataSegment)(eeboot_segmentDescriptor_t *dataSegment) {
    uint32_t bytesToWrite = dataSegment->length;
    uint32_t readPosition = dataSegment->sourceAddress;
    uint32_t writeAddress = dataSegment->destinationAddress;
    //CRC32 already verified
    //Verify alignment
    if ((bytesToWrite & (WRITE_PAGE_BYTES - 1)) || (writeAddress & (ERASE_PAGE_BYTES - 1))) {
        //Misaligned data request
        if ((bytesToWrite == 4) || (bytesToWrite == 8)) {
            //Special request to write vectors or CRC as last write. Only if input is formatted so
            if (eeboot_seekReadData(readPosition, bytesToWrite, intFlashSourceBuffer)) {
                write_dorqword(writeAddress, intFlashSourceBuffer, bytesToWrite == 8);
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
        uint32_t *verifyBuffer = (uint32_t*) (writeAddress | 0xA0000000); //Allocate to KSEG1 uncached segment
        int counter = WRITE_PAGE_BYTES / 4;
        while(counter != 0) {
            counter--;
            if (verifyBuffer[counter] != intFlashSourceBuffer[counter]) {
                return false;
            }
        }
    }
    return true;
}