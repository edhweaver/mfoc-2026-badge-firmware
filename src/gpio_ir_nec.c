/**
 * @file      gpio_ir_nec.c
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

#include "gpio_ir_nec.h"

//============================================================================
//
//   WS2812 Driver Functions
//
//      The functions in this file will manage the WS2812 LED via a GPO pin
//   that must be defined in the pin_config.h file defined by the keyword
//   WS2812_DIO_PIN.
//
//      The functions ws2812_block_until_safe is a weak function that will be
//   that will immedately return.  Because the WS2812 pulse train is timing
//   critical, this function can be replaced by a strong function in the
//   main project to block the pulse train output until it is safe to
//   generate.
//
//      The code is tuned to run on a 8 MHz clock.
//
//============================================================================

__attribute__((weak)) void ir_nec_block_until_safe() {
    ;
}

/* Initialize the IR Receiver pin */
void receiver_nec_set_pin() {

    // Set IR Receiver pin as input
    DDRB &= ~(1 << IR_RECEIVER_PIN);

    // Enable Pin Change Interrupts
    GIMSK |= (1 << PCIE);

    // Enable Pin Change Interrupt for IR Receiver Pin
    PCMSK |= (1 << IR_RECEIVER_INTERRUPT);
}

void nec_input_initialize(nec_code_parser_t * nec_input) {
    nec_input->parser_state = NEC_HEADING_00;
    nec_input->pin_state = 0;
    nec_input->address = 0;
    nec_input->command = 0;
}

void nec_input_parser(nec_code_parser_t * nec_input) {
    static uint8_t checksum;
    static uint16_t address_constructor;
    static uint8_t command_constructor;
    enum ir_nec_states next_state = NEC_HEADING_00;
    uint8_t pin_state;

    pin_state = nec_input->pin_state;

    if (pin_state & NEC_INPUT_EXCEEDED_TIME) {
        next_state = NEC_HEADING_00;
    }

    switch (nec_input->parser_state) {
        default:
        case NEC_HEADING_00:
            if (pin_state & NEC_INPUT_SIGNAL_LOW) {

                // Falling edge signal
                next_state = NEC_HEADING_00;
            } else if (pin_state & NEC_INPUT_HEADER_00) {

                // Rising edge signal was longer than 8.0 msec
                next_state = NEC_HEADING_01;
            } else {

                // Rising edge signal was shorter than 8.0 msec
                next_state = NEC_HEADING_00;
            }
            break;
        case NEC_HEADING_01:
            if (pin_state & NEC_INPUT_SIGNAL_LOW) {

                // IR Reciever Pin is falling edge
                if (pin_state & NEC_INPUT_HEADER_01) {

                    // Time since last signal was longer than 12.0 msec
                    next_state = NEC_ADDRESS_BIT_00;

                    address_constructor = 0x0000U;
                    command_constructor = 0x00U;
                } else {

                    // Time since last signal was shorter than 12.0 msec
                    next_state = NEC_HEADING_00;
                }
            } else {

                // IR Reciever Pin is rising edge
                next_state = NEC_HEADING_00;
            }
            break;
        case NEC_ADDRESS_BIT_00:
        case NEC_ADDRESS_BIT_01:
        case NEC_ADDRESS_BIT_02:
        case NEC_ADDRESS_BIT_03:
        case NEC_ADDRESS_BIT_04:
        case NEC_ADDRESS_BIT_05:
        case NEC_ADDRESS_BIT_06:
        case NEC_ADDRESS_BIT_07:
            if (pin_state & NEC_INPUT_SIGNAL_LOW) {

                // Update the state machine when the IR Receiver Pin is a
                //   falling edge signal.
                next_state = nec_input->parser_state + 1;

                // Shift the input reader right to prepare for the next bit
                address_constructor = address_constructor >> 1;

                // Add a most significant bit 1 for the long pulses
                if (pin_state & NEC_INPUT_DATA_BIT_1) {
                    address_constructor |= 0x0080U;
                }
            } else {
                next_state = nec_input->parser_state;
            }
            break;
        case NEC_ADDRESS_BIT_08:
        case NEC_ADDRESS_BIT_09:
        case NEC_ADDRESS_BIT_10:
        case NEC_ADDRESS_BIT_11:
        case NEC_ADDRESS_BIT_12:
        case NEC_ADDRESS_BIT_13:
        case NEC_ADDRESS_BIT_14:
        case NEC_ADDRESS_BIT_15:
            if (pin_state & NEC_INPUT_SIGNAL_LOW) {

                // Update the state machine when the IR Receiver Pin is a
                //   falling edge signal.
                next_state = nec_input->parser_state + 1;

                // Shift the input reader right to prepare for the next bit
                address_constructor = ((address_constructor & 0xFE00) >> 1) |
                                     (address_constructor & 0x00FF);

                // Add a most significant bit 1 for the long pulses
                if (pin_state & NEC_INPUT_DATA_BIT_1) {
                    address_constructor |= 0x8000U;
                }
            } else {
                next_state = nec_input->parser_state;
            }
            break;
        case NEC_COMMAND_BIT_00:
        case NEC_COMMAND_BIT_01:
        case NEC_COMMAND_BIT_02:
        case NEC_COMMAND_BIT_03:
        case NEC_COMMAND_BIT_04:
        case NEC_COMMAND_BIT_05:
        case NEC_COMMAND_BIT_06:
        case NEC_COMMAND_BIT_07:
            if (pin_state & NEC_INPUT_SIGNAL_LOW) {

                // Update the state machine when the IR Receiver Pin is a
                //   falling edge signal.
                next_state = nec_input->parser_state + 1;

                // Shift the input reader right to prepare for the next bit
                command_constructor = command_constructor >> 1;

                // Add a most significant bit 1 for the long pulses
                if (pin_state & NEC_INPUT_DATA_BIT_1) {
                    command_constructor |= 0x80U;
                }
            } else {
                next_state = nec_input->parser_state;
            }
            break;
        case NEC_INVERTED_COMMAND_BIT_00:
        case NEC_INVERTED_COMMAND_BIT_01:
        case NEC_INVERTED_COMMAND_BIT_02:
        case NEC_INVERTED_COMMAND_BIT_03:
        case NEC_INVERTED_COMMAND_BIT_04:
        case NEC_INVERTED_COMMAND_BIT_05:
        case NEC_INVERTED_COMMAND_BIT_06:
        case NEC_INVERTED_COMMAND_BIT_07:
            if (pin_state & NEC_INPUT_SIGNAL_LOW) {

                // Update the state machine when the IR Receiver Pin is a
                //   falling edge signal.
                next_state = nec_input->parser_state + 1;

                // Shift the input reader right to prepare for the next bit
                checksum = checksum >> 1;

                // Add a most significant bit 1 for the long pulses
                if (pin_state & NEC_INPUT_DATA_BIT_1) {
                    checksum |= 0x80U;
                }

                // Set the address and command values if the checksum is valid
                if (nec_input->parser_state == NEC_INVERTED_COMMAND_BIT_07) {
                    if (checksum ^ command_constructor) {
                        nec_input->address = address_constructor;
                        nec_input->command = command_constructor;
                    }
                }
            } else {
                next_state = nec_input->parser_state;
            }
            break;
        case NEC_TAIL_00:
            next_state = NEC_HEADING_00;
            break;
    }

    nec_input->parser_state = next_state;
}
