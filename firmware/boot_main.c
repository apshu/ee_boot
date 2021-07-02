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
#include "eeboot_source_iic_eep_bb.h"

#define EEPROM_A_POWER_OFF() do { TRISBSET = (1<<14) | 3; __delay_ms(50); } while(0)
#define EEPROM_A_POWER_ON() do { LATBCLR = 1<<14; TRISBCLR = 1<<14; __delay_ms(50); } while(0)
#define EEPROM_B_POWER_OFF() do { TRISBSET = 1<<14; TRISASET = 3; __delay_ms(50); } while(0)
#define EEPROM_B_POWER_ON() do { LATBCLR = 1<<14; TRISBCLR = 1<<14; __delay_ms(50); } while(0)

#define LED_RED() do { LATBCLR = 1<<13; LATBSET = 1<<15; TRISBCLR = (1<<15) + (1<<13); } while(0)
#define LED_GREEN()   do { LATBSET = 1<<13; LATBCLR = 1<<15; TRISBCLR = (1<<15) + (1<<13); } while(0)
#define LED_OFF()   do { TRISBSET = (1<<15) + (1<<13); } while(0)

#define ENABLE_BTN() do { ANSELBCLR = 1<<4; TRISBSET = 1<<4; CNPUBSET = 1<<4; } while(0)
#define IS_BTN_ACTIVE() (~PORTB & (1<<4))

#define SYS_FREQ 25000000UL // Running at 25MHz

__ramfunc__ void __delay_us(uint32_t usDelay) {
    // Get the current core timer value and mark as a start count for Usec.
    register uint32_t startCntms = _CP0_GET_COUNT();
    // Convert microseconds Usec into how many clock ticks it will take
    register uint32_t waitCntms = (usDelay * (SYS_FREQ / 2)) / 1000000UL;
    // Wait until Core Timer count reaches the number we calculated earlier
    while (_CP0_GET_COUNT() - startCntms < waitCntms) {
        continue;
    }
}

__longramfunc__ void __delay_ms(uint32_t msDelay) {
    // Get the current core timer value and mark as a start count for Usec.
    register uint32_t startCntms = _CP0_GET_COUNT();
    // Convert microseconds Usec into how many clock ticks it will take
    register uint32_t waitCntms = (msDelay * (SYS_FREQ / 2000UL));
    // Wait until Core Timer count reaches the number we calculated earlier
    while (_CP0_GET_COUNT() - startCntms < waitCntms) {
        continue;
    }
}

static void configureOscillator(void) {
    SYSKEY = 0x0; //write invalid key to force lock
    SYSKEY = 0xAA996655; //write Key1 to SYSKEY
    SYSKEY = 0x556699AA; //write Key2 to SYSKEY
    // ORPOL disabled; SIDL disabled; SRC USB; TUN Center frequency; POL disabled; ON enabled; 
    OSCTUN = 0x9000;
    // PLLODIV 1:4; PLLMULT 12x; PLLICLK FRC; 
    SPLLCON = 0x2050080;
    // SBOREN disabled; VREGS disabled; RETEN disabled; 
    PWRCON = 0x00;
    // CF No Clock Failure; FRCDIV FRC/1; SLPEN Device will enter Idle mode when a WAIT instruction is issued; NOSC SPLL; SOSCEN disabled; CLKLOCK Clock and PLL selections are not locked and may be modified; OSWEN Switch is Complete; 
    OSCCON = (0x100 | _OSCCON_OSWEN_MASK);
    SYSKEY = 0x0; //write invalid key to force lock
    // Wait for Clock switch to occur 
    while (OSCCONbits.OSWEN == 1) { continue; }
    while (CLKSTATbits.SPLLRDY != 1) { continue; }
    // ON disabled; DIVSWEN enabled; RSLP disabled; ROSEL SYSCLK; OE disabled; SIDL disabled; RODIV 1; 
    REFO1CON = 0x10200;
    // ROTRIM 0; 
    REFO1TRIM = 0x00;
}

static bool isBootNeeded(void) {
    if (*(uint32_t*) eeboot_APP_START_ADDRESS == 0xFFFFFFFF) {
        // Empty FLASH
        configureOscillator();
        return true;
    }
    ENABLE_BTN();
    __delay_ms(2); //Due to slower OSC, it will delay significantly more
    if (IS_BTN_ACTIVE()) {
        //Possible boot request
        configureOscillator();
        uint_fast8_t btnCtr = 0;
        uint_fast8_t timeCtr = 0;
        while (++timeCtr < 100) { //Timeout max. 10 seconds
            if (IS_BTN_ACTIVE()) {
                btnCtr++;
            } else {
                btnCtr = 0;
            }
            if (btnCtr > 50U) {
                //Bootload request for 5 continuous seconds
                for (btnCtr = 20; btnCtr; --btnCtr) {
                    if (IS_BTN_ACTIVE()) {
                        btnCtr = 20;
                    }
                    if (btnCtr & 1) {
                        LED_OFF();
                    } else {
                        LED_GREEN();
                    }
                    __delay_ms(100);
                }
                return true;
            }
            if (btnCtr) {
                if (timeCtr & 1) {
                    LED_GREEN();
                } else {
                    LED_RED();
                }
            } else {
                LED_OFF();
            }
            __delay_ms(100);
        }
    }
    return false;
}

__longramfunc__ static void doBoot(void) {
    LED_RED();
    //Init I2C
    EEPROM_A_POWER_ON();
    eeboot_initializeBitbangI2CPort('B', 1, 'B', 0);
    if (!eeboot_loadImage(0UL)) {
        //Do this on boot loader error
        EEPROM_A_POWER_OFF();
        EEPROM_B_POWER_ON();
        //Prepare secondary slot
        eeboot_initializeBitbangI2CPort('A', 0, 'A', 1);
        if (!eeboot_loadImage(0UL)) {
            //Second slot bootload not successful
            EEPROM_A_POWER_OFF();
            EEPROM_B_POWER_OFF();
            uint_fast8_t blinkctr;
            for (blinkctr = 0; blinkctr < 50; ++blinkctr) { // Blink "error" for 15sec
                //Blinking RED
                LED_OFF();
                __delay_ms(275); //Wait 125ms
                LED_RED();
                __delay_ms(25); //Wait 25ms
            }
            self_reset(); // initiate the reset
            while (1) {
                continue;
            };
        }
    }
    EEPROM_A_POWER_OFF();
    EEPROM_B_POWER_OFF();
    LED_GREEN();
    __delay_ms(3000); //Wait 3 seconds
}

int main(void) {
    // System unlock sequence
    __asm__ __volatile__ (" di");
    NVMKEY = 0;
    NVMKEY = 0xAA996655;
    NVMKEY = 0x556699AA;
    // All boot memory write protected
    NVMBWP = 7;
    NVMKEY = 0;
    if (isBootNeeded()) {
        doBoot();
        self_reset(); // initiate the reset
        while (1) {
            continue;
        };
    }
    jump_to_app();
}

