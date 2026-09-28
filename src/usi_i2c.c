/**
 * @file      usi_i2c.c
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     USI I2C Driver
 * @version   1.0
 * @date      2026-08-09
 *
 * @copyright Copyright (C) 2026 Ed Weaver
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://gnu.org>.
 */

#include "usi_i2c.h"

__attribute__((weak)) void i2c_write_value(uint8_t register_address, uint8_t index, uint8_t * incoming_data,  uint8_t * outgoing_data) {
    ;
}

__attribute__((weak)) void i2c_read_value(uint8_t register_address, uint8_t index, uint8_t * data) {
    data[0] = 0xFFU;
}


void i2c_initialize() {

    SAO_START

    SAO_INITIALIZE_PINS

    USI_CLEAR_ALL_ISR_FLAGS

    SAO_INTERRUPT_ON_START_CONDITION
}
