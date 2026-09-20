/**
 * @file      main.h
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     2026 Orange County Maker Faire Badge Source Code
 * @version   1.0
 * @date      2026-05-06
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

#ifndef MAIN_H
#define MAIN_H

#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "pin_config.h"
#include "gpio_ws2812.h"

//============================================================================
//
// Section: Macros for Flag Management
//
//      This section contains Macros for Setting, Clearing and Checking Flag
//   for asynchronus communication between an Interrupt Service Routine and
//   other functions.  All Macro calls outside of an ISR must have one of the
//   the following conditions to ensure that the Macros are Atomic:
//      - Interrupts are disabled prior to the call
//      - The Variable used by the macro is declared as a register
//
//      All Macro Calls inside an ISR are already Atomic due to further
//   Interrupts being disabled while executing an ISR in the AVR
//   hardware architecture.
//
//============================================================================

// Register used to capture flags
register uint8_t timer_1_flags __asm__("r2");
register uint8_t timer_2_flags __asm__("r3");

// Flag is Active when Switch 1 debounced state is High
#define SET_FLAG__SWITCH_Y_ON                       timer_1_flags |= (1 << 5);
#define CLEAR_FLAG__SWITCH_Y_ON                    timer_1_flags &= ~(1 << 5);
#define READ_FLAG__SWITCH_Y_ON                        timer_1_flags & (1 << 5)

// Flag is Active when Switch Y debounced state is Low
#define SET_FLAG__SWITCH_Y_OFF                      timer_1_flags |= (1 << 6);
#define CLEAR_FLAG__SWITCH_Y_OFF                   timer_1_flags &= ~(1 << 6);
#define READ_FLAG__SWITCH_Y_OFF                       timer_1_flags & (1 << 6)

// Flag is Active when Button X debounced state is High
#define SET_FLAG__BUTTON_X_PRESSED                  timer_2_flags |= (1 << 6);
#define CLEAR_FLAG__BUTTON_X_PRESSED               timer_2_flags &= ~(1 << 6);
#define READ_FLAG__BUTTON_X_PRESSED                   timer_2_flags & (1 << 6)

// Flag is Active when Button X debounced state is Low
#define SET_FLAG__BUTTON_X_RELEASED                 timer_2_flags |= (1 << 7);
#define CLEAR_FLAG__BUTTON_X_RELEASED              timer_2_flags &= ~(1 << 7);
#define READ_FLAG__BUTTON_X_RELEASED                  timer_2_flags & (1 << 7)

#define CLEAR_ALL_FLAGS                                  timer_1_flags = 0U; \
                                                           timer_2_flags = 0U;

//============================================================================

#endif /* MAIN_H */