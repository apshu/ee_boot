#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "eeboot.h"
#include "eeboot_source_iic_eep_bb.h"

#ifdef __XC8__
#define PORT_LENGTH_BYTES 1
#elif defined(__XC16__)
#define PORT_LENGTH_BYTES 2
#elif defined(__XC32__)
#define PORT_LENGTH_BYTES 4
#define ODCON ODC
#define WPU CNPU
#else 
#warning "Compiler not recognized"
#endif

typedef
#if PORT_LENGTH_BYTES == 1
uint8_t
#elif PORT_LENGTH_BYTES == 2
uint16_t
#elif PORT_LENGTH_BYTES == 4
uint32_t
#else
#error "SWI2C: GPIO size invalid, Check compiler defines."
#endif
gpiotype_t;

volatile gpiotype_t *swi2c_SDAport;
volatile gpiotype_t *swi2c_SDAlat;
gpiotype_t swi2c_SDApinmask;
volatile gpiotype_t *swi2c_SCLport;
volatile gpiotype_t *swi2c_SCLlat;
gpiotype_t swi2c_SCLpinmask;

#define swi2c_read_SCL() ((*swi2c_SCLport) & swi2c_SCLpinmask)
#define swi2c_read_SDA() ((*swi2c_SDAport) & swi2c_SDApinmask)
#define swi2c_set_SCL()   do { *swi2c_SCLlat |= swi2c_SCLpinmask; } while(0)
#define swi2c_clear_SCL() do { *swi2c_SCLlat &= ~swi2c_SCLpinmask; } while(0)
#define swi2c_set_SDA()   do { *swi2c_SDAlat |= swi2c_SDApinmask; } while(0)
#define swi2c_clear_SDA() do { *swi2c_SDAlat &= ~swi2c_SDApinmask; } while(0)
#define swi2c_arbitration_lost() do { return false; } while(0)
#define swi2c_halfbit_delay() do {__delay_us(1); } while(0)

static eeboot_ram_func bool i2c_start_cond(void) {
    if (swi2c_read_SDA() == 0) {
        swi2c_arbitration_lost();
        return false;
    }

    // SCL is high, set SDA from 1 to 0.
    swi2c_clear_SDA();
    swi2c_halfbit_delay();
    swi2c_clear_SCL();
    return true;
}

static eeboot_ram_func bool i2c_restart_cond(void) {
    // if started, do a restart condition
    // set SDA to 1
    swi2c_set_SDA();
    swi2c_halfbit_delay();
    swi2c_set_SCL();
    if (swi2c_read_SCL() == 0) { // Clock stretching detected
        swi2c_halfbit_delay();
        swi2c_halfbit_delay();
        if (swi2c_read_SCL() == 0) { // Clock stretching still active, abort
            swi2c_arbitration_lost();
            return false;
        }
    }

    // Repeated start setup time, minimum 4.7us
    swi2c_halfbit_delay();
    i2c_start_cond();
}

static eeboot_ram_func bool i2c_stop_cond(void) {
    // set SDA to 0
    swi2c_clear_SDA();
    swi2c_halfbit_delay();

    swi2c_set_SCL();
    // Clock stretching
    if (swi2c_read_SCL() == 0) { // Clock stretching detected
        swi2c_halfbit_delay();
        swi2c_halfbit_delay();
        if (swi2c_read_SCL() == 0) { // Clock stretching still active, abort
            swi2c_arbitration_lost();
            return false;
        }
    }

    // Stop bit setup time, minimum 4us
    swi2c_halfbit_delay();

    // SCL is high, set SDA from 0 to 1
    swi2c_set_SDA();
    swi2c_halfbit_delay();

    if (swi2c_read_SDA() == 0) {
        swi2c_arbitration_lost();
        return false;
    }
    return true;
}

// Write a bit to I2C bus

static eeboot_ram_func bool i2c_write_bit(bool bitval) {
    if (bitval) {
        swi2c_set_SDA();
    } else {
        swi2c_clear_SDA();
    }

    // SDA change propagation delay
    swi2c_halfbit_delay();

    // Set SCL high to indicate a new valid SDA value is available
    swi2c_set_SCL();

    // Wait for SDA value to be read by slave, minimum of 4us for standard mode
    swi2c_halfbit_delay();

    if (swi2c_read_SCL() == 0) { // Clock stretching detected
        swi2c_halfbit_delay();
        swi2c_halfbit_delay();
        if (swi2c_read_SCL() == 0) { // Clock stretching still active, abort
            swi2c_arbitration_lost();
            return false;
        }
    }

    // SCL is high, now data is valid
    // If SDA is high, check that nobody else is driving SDA
    if (bitval && (swi2c_read_SDA() == 0)) {
        swi2c_arbitration_lost();
        return false;
    }

    // Clear the SCL to low in preparation for next change
    swi2c_clear_SCL();
    return true;
}

// Read a bit from I2C bus

static eeboot_ram_func bool i2c_read_bit(void) {
    bool bitval;

    // Let the slave drive data
    swi2c_set_SDA();
    swi2c_halfbit_delay();

    // Set SCL high to indicate a new valid SDA value is available
    swi2c_set_SCL();
    swi2c_halfbit_delay();
    
    if (swi2c_read_SCL() == 0) { // Clock stretching detected
        swi2c_halfbit_delay();
        swi2c_halfbit_delay();
        if (swi2c_read_SCL() == 0) { // Clock stretching still active, abort
            swi2c_arbitration_lost();
            return false;
        }
    }


    // SCL is high, read out bit
    bitval = swi2c_read_SDA();

    // Set SCL low in preparation for next operation
    swi2c_clear_SCL();

    return (bitval);
}

// Write a byte to I2C bus. Return 0 if ack by the slave.

static eeboot_ram_func bool i2c_write_byte(uint_fast8_t byte) {
    uint_fast8_t bitnum;
    bool nack;

    for (bitnum = 0; bitnum < 8; ++bitnum) {
        if (!i2c_write_bit((byte & 0x80) != 0)) {
            return true;
        }
        byte <<= 1;
    }

    nack = i2c_read_bit();

    return nack;
}

// Read a byte from I2C bus

static eeboot_ram_func unsigned char i2c_read_byte(bool isnack) {
    uint_fast8_t databyte = 0;
    uint_fast8_t bitnum;

    for (bitnum = 0; bitnum < 8; ++bitnum) {
        databyte = (databyte << 1) | (i2c_read_bit() ? 1 : 0);
    }

    i2c_write_bit(isnack);

    return databyte;
}

#define __swi2c_getregptr(thereg, elem) (((volatile gpiotype_t*)&thereg##A) + ((&thereg##B - &thereg##A) * (elem)))
#define _swi2c_getregptr(thereg, elem) __swi2c_getregptr(thereg, elem)
#define swi2c_getregptr(thereg, elem) _swi2c_getregptr(thereg, elem)

eeboot_weak_ram_func bool EEBOOT_SOURCE_IICEEPBB_NAMESPACE(initializeBitbangI2CPort)(char SDAportletter, uint_least8_t SDAportbit, char SCLportletter, uint_least8_t SCLportbit) {
    assert(sizeof (PORTA) == PORT_LENGTH_BYTES); //Check if compile configuration matches architecture. Check happens only in debug mode
    SDAportletter |= 'a' - 'A';
    SCLportletter |= 'a' - 'A';
    if (EEBOOT_SOURCE_IICEEPBB_NAMESPACE(selectBitbangI2CPort)(SDAportletter, SDAportbit, SCLportletter, SCLportbit)) {
        *swi2c_getregptr(ODCON, SDAportletter - 'a') |= swi2c_SDApinmask;
        *swi2c_getregptr(ODCON, SCLportletter - 'a') |= swi2c_SCLpinmask;
        swi2c_set_SCL();
        swi2c_set_SDA();
        *swi2c_getregptr(WPU, SDAportletter - 'a') |= swi2c_SDApinmask;
        *swi2c_getregptr(WPU, SCLportletter - 'a') |= swi2c_SCLpinmask;
        *swi2c_getregptr(ANSEL, SDAportletter - 'a') &= ~swi2c_SDApinmask;
        *swi2c_getregptr(ANSEL, SCLportletter - 'a') &= ~swi2c_SCLpinmask;
        *swi2c_getregptr(TRIS, SDAportletter - 'a') &= ~swi2c_SDApinmask;
        *swi2c_getregptr(TRIS, SCLportletter - 'a') &= ~swi2c_SCLpinmask;
        uint_fast8_t ctr;
        for (ctr = 10; ctr; --ctr) {
            if (!swi2c_read_SCL()) {
                //Clock stuck low, hw reset needed
                break;
            }
            if (swi2c_read_SDA()) {
                //Force a stop condition
                swi2c_set_SCL();
                swi2c_halfbit_delay();
                swi2c_clear_SDA();
                swi2c_halfbit_delay();
                swi2c_set_SDA();
                swi2c_halfbit_delay();
                break;
            }
            //Do clock cycle to finish any pending read operation from slave devices out of sync
            swi2c_clear_SCL();
            swi2c_halfbit_delay();
            swi2c_set_SCL();
            swi2c_halfbit_delay();
        }
        return (swi2c_read_SCL() && swi2c_read_SDA()); //If both I2C lines are high - bus is idle and the init was successful
    }
    return false;
}

eeboot_weak_ram_func bool EEBOOT_SOURCE_IICEEPBB_NAMESPACE(selectBitbangI2CPort)(char SDAportletter, uint_least8_t SDAportbit, char SCLportletter, uint_least8_t SCLportbit) {
    SDAportletter |= 'a' - 'A';
    SCLportletter |= 'a' - 'A';
    if ((SDAportletter >= 'a') && (SDAportletter <= 'f') && (SDAportbit < (1 << PORT_LENGTH_BYTES)) &&
            (SCLportletter >= 'a') && (SCLportletter <= 'f') && (SCLportbit < (1 << PORT_LENGTH_BYTES))) {
        swi2c_SDAport = swi2c_getregptr(PORT, SDAportletter - 'a');
        swi2c_SDAlat = swi2c_getregptr(LAT, SDAportletter - 'a');
        swi2c_SDApinmask = 1 << SDAportbit;
        swi2c_SCLport = swi2c_getregptr(PORT, SCLportletter - 'a');
        swi2c_SCLlat = swi2c_getregptr(LAT, SCLportletter - 'a');
        swi2c_SCLpinmask = 1 << SCLportbit;
        return true;
    }
    return false;
}

static eeboot_ram_func bool eeboot_setEEPROMReadAddress(uint32_t eepWriteAddress) {

    union {
        uint32_t longAddr;

        struct {
            uint32_t lbyte : 8;
            uint32_t hbyte : 8;
            uint32_t ubyte : 8;
            uint32_t ____unused : 8;
        };
    } iic_address;

    assert(swi2c_SDAport); //Check during debugging if the INIT function is called!
    iic_address.longAddr = (eepWriteAddress & 0x0007FFFF) | 0x00500000; //Mask to 18 bits, add EEPROM IIC address identifier
    iic_address.ubyte <<= 1; //Make place for R/~W
    if (i2c_start_cond()) {
        if (!i2c_write_byte(iic_address.ubyte)) {
            if (!i2c_write_byte(iic_address.hbyte)) {
                if (!i2c_write_byte(iic_address.lbyte)) {
                    if (i2c_stop_cond()) {
                        if (i2c_start_cond()) {
                            if (!i2c_write_byte(iic_address.ubyte | 1)) { //Issue read command
                                return true;
                            }
                        }
                    } else {
                        return false;
                    }
                }
            }
        }
        i2c_stop_cond();
    }
    return false;
}

eeboot_ram_func bool EEBOOT_SOURCE_IICEEPBB_NAMESPACE(seekReadData)(uint32_t offset, size_t numBytes, void* dataBuffer) {
    if (eeboot_setEEPROMReadAddress(offset)) {
        //Read address set, read mode engaged
        uint8_t *dataBuf = (uint8_t*) dataBuffer;
        while (numBytes--) {
            *dataBuf++ = i2c_read_byte(!numBytes); //Read a lot of bytes, while NACK only the last byte
        }
        if (i2c_stop_cond()) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

eeboot_ram_func uint32_t EEBOOT_SOURCE_IICEEPBB_NAMESPACE(getCRCseekRead)(uint32_t crc32StartValue, uint32_t crc32Poly, uint32_t offset, size_t numBytes) {
    if (eeboot_setEEPROMReadAddress(offset)) {
        //Read address set, read mode engaged
        int k;
        while (numBytes--) {
            crc32StartValue ^= i2c_read_byte(!numBytes);
            for (k = 0; k < 8; k++)
                crc32StartValue = (crc32StartValue & 1) ? ((crc32StartValue >> 1) ^ crc32Poly) : (crc32StartValue >> 1);
        }
        i2c_stop_cond();
    }
    return crc32StartValue;
}