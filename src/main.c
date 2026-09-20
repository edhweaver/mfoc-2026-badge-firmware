/**
 * @file      main.c
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

#include <avr/interrupt.h>
#include <util/delay.h>
#include "pin_config.h"

//============================================================================
//
//   Phase 1: Blink LED on PB4 (pin 3) with AVR C Code
//
//      Blink LED connected to PB4 (Pin 3) once per second.
//
//============================================================================

int main(void) {

    // Set Blinking LED as output
    DDRB |= (1 << BLINK_LED_PIN);

    while(1) {

        // Toggle Blinking LED
        PORTB ^= (1 << BLINK_LED_PIN);

        // Wait for 500 milliseconds
        _delay_ms(500);
    }
}
