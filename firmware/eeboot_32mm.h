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

#if !defined(eeboot_APP_START_ADDRESS)
#define eeboot_APP_START_ADDRESS (0x9D000000UL)
#endif
    
#define eeboot_ram_func __longramfunc__ __attribute__((weak))
#define jump_to_app() do { __asm__ __volatile__("\tla $t0,%0\n" "\tjr $t0": : "" (eeboot_APP_START_ADDRESS) ); } while(0)

#ifdef	__cplusplus
}
#endif

#endif	/* EEBOOT_32MM_H */

