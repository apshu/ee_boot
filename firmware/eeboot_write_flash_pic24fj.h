/* 
 * File:   eeboot_write_flash_pic24fj.h
 * Author: 
 *
 * Created on July 5, 2021, 11:00 AM
 */

#ifndef EEBOOT_WRITE_FLASH_PIC24FJ_H
#define	EEBOOT_WRITE_FLASH_PIC24FJ_H

#ifdef	__cplusplus
extern "C" {
#endif


#if !defined(EEBOOT_WRITE_INTFLASH_NAMESPACE)
#define EEBOOT_WRITE_INTFLASH_NAMESPACE(func_name) eeboot_##func_name
#endif

    eeboot_ram_func bool EEBOOT_WRITE_INTFLASH_NAMESPACE(storeDataSegment)(eeboot_segmentDescriptor_t *dataSegment);

#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_WRITE_FLASH_PIC24FJ_H */

