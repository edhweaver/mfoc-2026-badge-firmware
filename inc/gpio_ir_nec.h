/**
 * @file      gpio_ir_nec.h
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     NEC Frame IR LED Driver
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

#ifndef GPIO_IR_NEC_H
#define GPIO_IR_NEC_H

#include <avr/interrupt.h>
#include <util/delay.h>
#include "pin_config.h"

#define NEC_INPUT_HEADER_00                                               0x01
#define NEC_INPUT_HEADER_01                                               0x02
#define NEC_INPUT_DATA_BIT_1                                              0x04
#define NEC_INPUT_EXCEEDED_TIME                                           0x08
#define NEC_INPUT_SIGNAL_LOW                                              0x10
#define NEC_INPUT_SIGNAL_HIGH                                             0x20

// States for tracking each section of NEC Frame
enum ir_nec_states {
    NEC_MONITORING_DISABLED = 0,
    NEC_ACTIVE_MONITORING = 1,
    NEC_HEADING_00 = 2,
    NEC_HEADING_01 = 3,
    NEC_ADDRESS_BIT_00 = 4,
    NEC_ADDRESS_BIT_01 = 5,
    NEC_ADDRESS_BIT_02 = 6,
    NEC_ADDRESS_BIT_03 = 7,
    NEC_ADDRESS_BIT_04 = 8,
    NEC_ADDRESS_BIT_05 = 9,
    NEC_ADDRESS_BIT_06 = 10,
    NEC_ADDRESS_BIT_07 = 11,
    NEC_ADDRESS_BIT_08 = 12,
    NEC_ADDRESS_BIT_09 = 13,
    NEC_ADDRESS_BIT_10 = 14,
    NEC_ADDRESS_BIT_11 = 15,
    NEC_ADDRESS_BIT_12 = 16,
    NEC_ADDRESS_BIT_13 = 17,
    NEC_ADDRESS_BIT_14 = 18,
    NEC_ADDRESS_BIT_15 = 19,
    NEC_COMMAND_BIT_00 = 20,
    NEC_COMMAND_BIT_01 = 21,
    NEC_COMMAND_BIT_02 = 22,
    NEC_COMMAND_BIT_03 = 23,
    NEC_COMMAND_BIT_04 = 24,
    NEC_COMMAND_BIT_05 = 25,
    NEC_COMMAND_BIT_06 = 26,
    NEC_COMMAND_BIT_07 = 27,
    NEC_INVERTED_COMMAND_BIT_00 = 28,
    NEC_INVERTED_COMMAND_BIT_01 = 29,
    NEC_INVERTED_COMMAND_BIT_02 = 30,
    NEC_INVERTED_COMMAND_BIT_03 = 31,
    NEC_INVERTED_COMMAND_BIT_04 = 32,
    NEC_INVERTED_COMMAND_BIT_05 = 33,
    NEC_INVERTED_COMMAND_BIT_06 = 34,
    NEC_INVERTED_COMMAND_BIT_07 = 35,
    NEC_TAIL_00 = 36
};

typedef struct {
    enum ir_nec_states parser_state;
    uint8_t pin_state;
    uint16_t address;
    uint8_t command;
} nec_code_parser_t;

void ir_nec_set_pin();
void ir_nec_send(uint16_t address, uint8_t command);
void ir_nec_block_until_safe();
void receiver_nec_set_pin();
void nec_input_initialize(nec_code_parser_t * nec_input);
void nec_input_parser(nec_code_parser_t * nec_input);

#endif /* GPIO_IR_NEC_H */