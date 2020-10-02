#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include "eeboot.h"

eeboot_weak_ram_func bool eeboot_seekReadData(uint32_t offset, size_t num_bytes, void* data_buffer) {
    return false;
}

eeboot_weak_ram_func uint32_t eeboot_getCRCseekRead(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, size_t numBytes) {
    return 0UL;
}

eeboot_weak_ram_func bool eeboot_isBootNeeded(void) {
    return false;
}

eeboot_weak_ram_func bool eeboot_isImageCompatible(eeboot_bootfileFixData_t *compatData) {
    //If compatibility check required, or newer version than supported, fail loading image. 
    //If checks are required, user code has to do the check logic
    return !(compatData->DEVID0_populated || compatData->DEVID1_populated || compatData->DEVID2_populated || compatData->PCBID0_populated || compatData->PCBID1_populated || compatData->PCBID2_populated || (compatData->version != 0));
}

eeboot_weak_ram_func bool eeboot_storeDataSegment(eeboot_segmentDescriptor_t *dataSegment) {
    return false;
}

eeboot_ram_func bool eeboot_loadImage(uint32_t inputDataOffsetAddress) {
    eeboot_fileHeader_t fileHdr;
    if (eeboot_seekReadData(inputDataOffsetAddress, sizeof (fileHdr), &fileHdr)) {
        //File header successfully read
        if ((fileHdr.length >= 43) && (fileHdr.contentIdentifier == 0x544F4F42UL)) {
            //Boot image file recognized
            if (fileHdr.CRC32 == (eeboot_getCRCseekRead(eeboot_FILE_HEADER_CRC32_START_VALUE, eeboot_FILE_HEADER_CRC32_POLY, inputDataOffsetAddress + 4, fileHdr.length - 4) ^ eeboot_FILE_HEADER_CRC32_XOR_VALUE)) {
                //File header successfully verified CRC32
                eeboot_bootfileFixData_t bootHdr;
                if (eeboot_seekReadData(inputDataOffsetAddress + sizeof (fileHdr), sizeof (bootHdr), &bootHdr)) {
                    //Bootload compatibility info successfully read
                    if (eeboot_isImageCompatible(&bootHdr)) {
                        //CRC32 valid header and compatible with the system
                        uint32_t headerReadPosition = inputDataOffsetAddress + sizeof (fileHdr) + sizeof (bootHdr);
                        do {
                            eeboot_segmentDescriptor_t dataSegmentDescriptor;
                            if (eeboot_seekReadData(headerReadPosition, sizeof (dataSegmentDescriptor), &dataSegmentDescriptor)) {
                                if ((dataSegmentDescriptor.dataCRC32 == 0xFFFFFFFF) && (dataSegmentDescriptor.destinationAddress == 0xFFFFFFFF) && (dataSegmentDescriptor.sourceAddress == 0xFFFFFFFF) && (dataSegmentDescriptor.length == 0xFFFFFFFF)) {
                                    //Terminator segment, and no errors ended the programming -> it's a successful programming
                                    return true;
                                }
                                dataSegmentDescriptor.sourceAddress += inputDataOffsetAddress;
                                if (dataSegmentDescriptor.dataCRC32 == (eeboot_getCRCseekRead(eeboot_DATA_SEGMENT_CRC32_START_VALUE, eeboot_DATA_SEGMENT_CRC32_POLY, dataSegmentDescriptor.sourceAddress, dataSegmentDescriptor.length) ^ eeboot_DATA_SEGMENT_CRC32_XOR_VALUE)) {
                                    //CRC32 for the programmable data is valid

                                    if (eeboot_storeDataSegment(&dataSegmentDescriptor)) {
                                        //Successfully stored and verified this segment
                                        headerReadPosition += sizeof (dataSegmentDescriptor); //skip to next segment descriptor
                                        continue;
                                    }
                                }
                            }
                            break;
                        } while (1);
                    }
                }
            }
        }
    }
    return false;
}
