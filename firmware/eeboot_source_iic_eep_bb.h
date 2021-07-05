/* 
 * File:   eeboot_source_iic_eep_bb.h
 * Author: M91541
 *
 * Created on October 2, 2020, 2:13 PM
 */

#ifndef EEBOOT_SOURCE_IIC_EEP_BB_H
#define	EEBOOT_SOURCE_IIC_EEP_BB_H

#include "eeboot_platform.h"


#ifdef	__cplusplus
extern "C" {
#endif

#if !defined(EEBOOT_SOURCE_IICEEPBB_NAMESPACE)
#define EEBOOT_SOURCE_IICEEPBB_NAMESPACE(func_name) eeboot_##func_name
#endif

    eeboot_ram_func bool EEBOOT_SOURCE_IICEEPBB_NAMESPACE(initializeBitbangI2CPort)(char SDAportletter, uint_least8_t SDAportbit, char SCLportletter, uint_least8_t SCLportbit);
    eeboot_ram_func bool EEBOOT_SOURCE_IICEEPBB_NAMESPACE(selectBitbangI2CPort)(char SDAportletter, uint_least8_t SDAportbit, char SCLportletter, uint_least8_t SCLportbit);
    eeboot_ram_func bool EEBOOT_SOURCE_IICEEPBB_NAMESPACE(seekReadData)(uint32_t offset, uint32_t numBytes, void* dataBuffer);
    eeboot_ram_func uint32_t EEBOOT_SOURCE_IICEEPBB_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, uint32_t numBytes);

#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_SOURCE_IIC_EEP_BB_H */

