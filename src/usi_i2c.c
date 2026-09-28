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

void i2c_initialize() {

    SAO_START

    SAO_INITIALIZE_PINS

    USI_CLEAR_ALL_ISR_FLAGS

    SAO_INTERRUPT_ON_START_CONDITION
}
