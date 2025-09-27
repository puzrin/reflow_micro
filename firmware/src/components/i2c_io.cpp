#include <pd/pd.h>

#include "i2c_io.hpp"

//
// Trivial proxy to reuse I2c from USB PD driver. Can be done better, but
// keep things simple for now.
//
// Ensure to init USB PD before using these functions.
//

extern pd::fusb302::Fusb302RtosHalEsp32 fusb302_hal;

void i2c_init() {
    fusb302_hal.init_i2c();
}

bool i2c_read_block(uint8_t i2c_addr, uint8_t reg, uint8_t *data, uint32_t size) {
    // Implementation for reading a block of data over I2C
    return fusb302_hal.read_block(i2c_addr, reg, data, size);
}

bool i2c_write_block(uint8_t i2c_addr, uint8_t reg, const uint8_t *data, uint32_t size) {
    // Implementation for writing a block of data over I2C
    return fusb302_hal.write_block(i2c_addr, reg, data, size);
}
