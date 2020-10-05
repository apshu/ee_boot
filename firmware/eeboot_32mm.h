/* 
 * File:   eeboot_32mm.h
 * Author: M91541
 *
 * Created on September 30, 2020, 4:53 PM
 */

#ifndef EEBOOT_32MM_H
#define	EEBOOT_32MM_H

#ifdef	__cplusplus
extern "C" {
#endif
#include <sys/attribs.h>

#if !defined(eeboot_APP_START_ADDRESS)
#define eeboot_APP_START_ADDRESS (0x9D000000UL)
#endif

#define eeboot_ram_func __ramfunc__
#define eeboot_weak_ram_func eeboot_ram_func __attribute__((weak))
#define jump_to_app() do { __asm__ __volatile__("\tla $t0,%0\n" "\tjr $t0": : "" (eeboot_APP_START_ADDRESS) ); } while(0)
#define self_reset() do { SYSKEY = 0; SYSKEY = 0xAA996655; SYSKEY = 0x556699AA; RSWRST = 1; unsigned long int bitBucket = RSWRST; } while(0)
#if defined(NDEBUG) || !defined(__DEBUG)
#define assert(X) do { ((void)0); } while(0)
#else
#define assert(X) do {((X) ? (void) 0 : __builtin_software_breakpoint()); } while(0)
#endif
    
#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_32MM_H */

