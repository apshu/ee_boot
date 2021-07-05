/* 
 * File:   eeboot_source_stream.h
 * Author: M91541
 *
 * Created on May 20, 2021, 8:27 AM
 */

#ifndef EEBOOT_SOURCE_STREAM_H
#define	EEBOOT_SOURCE_STREAM_H

#include "eeboot_platform.h"

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(EEBOOT_SOURCE_STREAM_NAMESPACE)
#define EEBOOT_SOURCE_STREAM_NAMESPACE(func_name) eeboot_##func_name
#endif
    
    typedef uint16_t (*stream_getch_func)(void);      //Low byte returns data, high byte defines error. If no error, HB=0. Irreversible error code = 0xFF
    typedef uint8_t (*stream_putch_func)(uint8_t);    //Returns 0 if byte successfully put to queue, error code otherwise. Error code 0xFF means irreversible error
    
    eeboot_ram_func bool EEBOOT_SOURCE_STREAM_NAMESPACE(selectStream)(stream_getch_func getchFunction, stream_putch_func putchFunction);
    eeboot_ram_func bool EEBOOT_SOURCE_STREAM_NAMESPACE(seekReadData)(uint32_t offset, uint32_t numBytes, void* dataBuffer);
    eeboot_ram_func uint32_t EEBOOT_SOURCE_STREAM_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, uint32_t numBytes);
    eeboot_ram_func uint8_t EEBOOT_SOURCE_STREAM_NAMESPACE(sendDword)(uint32_t dw);

#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_SOURCE_STREAM_H */

