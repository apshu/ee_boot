#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/attribs.h>

#include "eeboot.h"


eeboot_weak_far_ram_func bool eeboot_seekReadData(uint32_t offset, size_t num_bytes, void* data_buffer) {
    return false;
}

eeboot_weak_far_ram_func uint32_t eeboot_getCRCseekRead(uint32_t offset, size_t num_bytes) {
    return 0UL;
}

eeboot_weak_far_ram_func bool eeboot_isBootNeeded(void) {
    return false;
}

eeboot_weak_far_ram_func bool eeboot_isImageCompatible(eeboot_bootfileFixData_t *compatData) {
    return false;
}

eeboot_weak_far_ram_func bool eeboot_storeDataSegment(eeboot_segmentDescriptor_t *dataSegment) {
    return false;
}

eeboot_weak_far_ram_func bool eeboot_loadImage(uint32_t startAddress) {
    eeboot_fileHeader_t fileHdr;
    if (eeboot_seekReadData(startAddress, sizeof(fileHdr), &fileHdr)) {
        //File header successfully read
        if ((fileHdr.length >= 43) && (fileHdr.contentIdentifier == 0x544F4F42UL)) {
            //Boot image file recognized
            if (fileHdr.CRC32 == eeboot_getCRCseekRead(4, fileHdr.length - 4)) {
                //File header successfully verified CRC32
                eeboot_bootfileFixData_t bootHdr;
                if (eeboot_seekReadData(startAddress + sizeof(fileHdr), sizeof(bootHdr), &bootHdr)) {
                    //Bootload compatibility info successfully read
                    if (eeboot_isImageCompatible(&bootHdr)) {
                        //CRC32 valid header and compatible with the system
                        uint32_t headerReadPosition = sizeof(fileHdr) + sizeof(bootHdr);
                        do {
                            eeboot_segmentDescriptor_t dataSegmentDescriptor;
                            if (eeboot_seekReadData(headerReadPosition, sizeof(dataSegmentDescriptor), &dataSegmentDescriptor)) {
                                if ((dataSegmentDescriptor.dataCRC32 == 0xFFFFFFFF) && (dataSegmentDescriptor.destinationAddress == 0xFFFFFFFF) && (dataSegmentDescriptor.sourceAddress == 0xFFFFFFFF) && (dataSegmentDescriptor.length == 0xFFFFFFFF)) {
                                    //Terminator segment, and no errors ended the programming -> it's a successful programming
                                    return true;
                                }
                                if (dataSegmentDescriptor.dataCRC32 == eeboot_getCRCseekRead(dataSegmentDescriptor.sourceAddress, dataSegmentDescriptor.length)) {
                                    //CRC32 for the programmable data is valid
                                    if (eeboot_storeDataSegment(&dataSegmentDescriptor)) {
                                        //Successfully stored and verified this segment
                                        continue;
                                    }
                                }
                            }
                            break;
                        } while(1);
                    }
                }
            }
        }
    }
    return false;
}
