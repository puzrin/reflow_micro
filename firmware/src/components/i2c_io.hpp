#pragma once

#include <stdint.h>

void i2c_init();
bool i2c_read_block(uint8_t i2c_addr, uint8_t reg, uint8_t *data, uint32_t size);
bool i2c_write_block(uint8_t i2c_addr, uint8_t reg, const uint8_t *data, uint32_t size);
