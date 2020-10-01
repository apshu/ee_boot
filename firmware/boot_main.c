/* 
 * File:   boot_main.c
 * Author: M91541
 *
 * Created on September 29, 2020, 3:53 PM
 */

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/attribs.h>
#include "eeboot.h"

__longramfunc__ int main(void) {
    if (eeboot_isBootNeeded()) {
        if (!eeboot_loadImage(0UL)) {
            //Do this on boot loader error
            while (1) {
                continue;
            }
        }
        SYSKEY = 0; // force lock
        SYSKEY = 0xAA996655; // unlock
        SYSKEY = 0x556699AA;
        RSWRST = 1;
        unsigned long int bitBucket = RSWRST;
        // initiate the reset
        while(1);
    }
    jump_to_app();
}

