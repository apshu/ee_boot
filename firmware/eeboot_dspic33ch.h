/* 
 * File:   eeboot_dspic33ch.h
 * Author: M91541
 *
 * Created on September 30, 2020, 4:53 PM
 */

#ifndef EEBOOT_DSPIC33CH_H
#define	EEBOOT_DSPIC33CH_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef __longramfunc__
#define __longramfunc__
#endif
    
#if !defined(eeboot_APP_START_ADDRESS)
#define eeboot_APP_START_ADDRESS (0xF800UL)
#endif

#define OPEN_DRAIN             ODC

#define eeboot_ram_func
#define eeboot_weak_ram_func eeboot_ram_func __attribute__((weak))
#define jump_to_app() do { void (*user_application)(void); user_application = (void(*)(void))eeboot_APP_START_ADDRESS; user_application(); } while(0)
#define self_reset() do { asm("\treset"); } while(0)
#if defined(NDEBUG) || !defined(__DEBUG)
#define assert(X) do { ((void)0); } while(0)
#else
#define assert(X) do {((X) ? (void) 0 : __builtin_software_breakpoint()); } while(0)
#endif
    
#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_DSPIC33CH_H */
