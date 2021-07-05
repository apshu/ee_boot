/* 
 * File:   eeboot_platform.h
 * Author: M91541
 *
 * Created on September 30, 2020, 5:02 PM
 */

#ifndef EEBOOT_PLATFORM_H
#define	EEBOOT_PLATFORM_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef __XC32__
#include "eeboot_32mm.h"
#elif defined __XC16__
#define FCY 90000000UL // Running at 90MIPS
#include <string.h>
#include <libpic30.h>
#include "eeboot_dspic33ch.h"
    
#define EEBOOT_SOURCE_IICEEPBB_NAMESPACE(func_name) eeboot_iicbb_##func_name
#define EEBOOT_SOURCE_STREAM_NAMESPACE(func_name) eeboot_stream_##func_name
#define EEBOOT_SOURCE_INTFLASH_NAMESPACE(func_name) eeboot_intflash_##func_name
    
#else
#error "Unsupported target"
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_PLATFORM_H */

