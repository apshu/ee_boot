/* 
 * File:   eeboot.h
 * Author: M91541
 *
 * Created on September 29, 2020, 3:54 PM
 */

#ifndef EEBOOT_H
#define	EEBOOT_H

#ifdef	__cplusplus
extern "C" {
#endif
    #include "eeboot_platform.h"
    
    typedef struct {
        uint32_t CRC32;
        uint32_t length;
        uint32_t contentIdentifier;
    } eeboot_fileHeader_t;
    
    typedef struct {
        uint32_t PCBID0_valid :1;
        uint32_t PCBID1_valid :1;
        uint32_t PCBID2_valid :1;
        uint32_t DEVID0_valid :1;
        uint32_t DEVID1_valid :1;
        uint32_t DEVID2_valid :1;
        uint32_t version :2;
        uint32_t PCBID0 :8;
        uint32_t PCBID1 :8;
        uint32_t PCBID2 :8;
        uint32_t DEVID0;
        uint32_t DEVID1;
        uint32_t DEVID2;
    } eeboot_bootfileFixData_t;
    
    typedef struct {
        uint32_t destinationAddress;
        uint32_t sourceAddress;
        uint32_t length;
        uint32_t dataCRC32;
    } eeboot_segmentDescriptor_t;
    
    typedef enum {
        eeboot_FILE_HEADER_CRC32_POLY         = 0x4C11DB7,
        eeboot_FILE_HEADER_CRC32_START_VALUE  = 0x544F4F42,
        eeboot_FILE_HEADER_CRC32_XOR_VALUE    = 0,
        eeboot_DATA_SEGMENT_CRC32_POLY        = eeboot_FILE_HEADER_CRC32_POLY,
        eeboot_DATA_SEGMENT_CRC32_START_VALUE = eeboot_FILE_HEADER_CRC32_START_VALUE,
        eeboot_DATA_SEGMENT_CRC32_XOR_VALUE   = eeboot_FILE_HEADER_CRC32_XOR_VALUE,
    } eeboot_crc32_e;
    
    eeboot_ram_func bool eeboot_isBootNeeded(void);
    eeboot_ram_func bool eeboot_loadImage(uint32_t startAddress);
    eeboot_ram_func bool eeboot_seekReadData(uint32_t offset, size_t numBytes, void* dataBuffer);
    eeboot_ram_func uint32_t eeboot_getCRCseekRead(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, size_t numBytes);
    eeboot_ram_func bool eeboot_isBootNeeded(void);
    eeboot_ram_func bool eeboot_isImageCompatible(eeboot_bootfileFixData_t *compatData);
    eeboot_ram_func bool eeboot_storeDataSegment(eeboot_segmentDescriptor_t *dataSegment);
    
#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_H */

