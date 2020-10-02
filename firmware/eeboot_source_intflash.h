/* 
 * File:   eeboot_source_intflash.h
 * Author: M91541
 *
 * Created on October 1, 2020, 12:10 PM
 */

#ifndef EEBOOT_SOURCE_INTFLASH_H
#define	EEBOOT_SOURCE_INTFLASH_H

#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(EEBOOT_SOURCE_INTFLASH_NAMESPACE)
#define EEBOOT_SOURCE_INTFLASH_NAMESPACE(func_name) eeboot_##func_name
#endif
    
    eeboot_ram_func bool EEBOOT_SOURCE_INTFLASH_NAMESPACE(seekReadData)(uint32_t offset, size_t numBytes, void* dataBuffer);
    eeboot_ram_func uint32_t EEBOOT_SOURCE_INTFLASH_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, size_t numBytes);
    
#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_SOURCE_INTFLASH_H */

