/* 
 * File:   eeboot_write_flash_pic32m.h
 * Author: M91541
 *
 * Created on October 2, 2020, 10:26 AM
 */

#ifndef EEBOOT_WRITE_FLASH_PIC32M_H
#define	EEBOOT_WRITE_FLASH_PIC32M_H

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

#endif	/* EEBOOT_WRITE_FLASH_PIC32M_H */

