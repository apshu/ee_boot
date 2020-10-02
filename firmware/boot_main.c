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

eeboot_ram_func bool eeboot_isBootNeeded(void) {
    return true;
}

__longramfunc__ int main(void) {
    if (eeboot_isBootNeeded()) {
        if (!eeboot_loadImage(0UL)) {
            //Do this on boot loader error
            while (1) {
                continue;
            }
        }
        self_reset(); // initiate the reset
        while (1) {
            continue;
        };
    }
    jump_to_app();
}

