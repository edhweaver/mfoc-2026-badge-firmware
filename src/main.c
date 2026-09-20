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
#include <avr/sleep.h>
#include "pin_config.h"

//============================================================================
//
//   Phase 2: Blink LED with Timer0 on Super Loop Architecture
//
//      Create a Super Loop Architecture with Timer 0 incrementing the loop
//   counter.  Set loop period to 200 milliseconds.  Blink LED connected to
//   PB4 (Pin 3) once per loop.
//
//============================================================================

// Loop count set to 200ms (25 interrupts at 8ms each)
#define LOOP_COUNT 25

// Loop counter incremented by Timer 0 interrupt
volatile uint8_t loop_counter = 0;

/* Timer/Counter0 Compare Match A */
ISR(TIMER0_COMPA_vect) {
    if (loop_counter < 255U) {
        loop_counter++;
    }
}

/* Initialize Timer0 */
void timer0_init() {
    // Set CTC Mode (WGM01=1, WGM00=0)
    TCCR0A = (1 << WGM01);

    // Set the compare value for 8ms
    // Processor Clock: 8MHz
    // Prescaler: 256
    // (8MHz / (256 * 125Hz)) - 1 = 249
    OCR0A = 249;

    // Enable the Compare Match A interrupt
    TIMSK |= (1 << OCIE0A);

    // Set Prescaler to 256 and start the timer
    // CS02 = 1, CS01 = 0, CS00 = 0
    TCCR0B = (1 << CS02);
}


int main(void) {

    //initialize Timer0 for 8ms interrupts
    timer0_init();

    // set processor to sleep mode idle to save power between interrupts
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Initialize the loop counter
    loop_counter = 0;

    // Set Blinking LED as output
    DDRB |= (1 << BLINK_LED_PIN);

    // Enable global interrupts
    sei();

    while(1) {

        // Toggle Blinking LED
        PORTB ^= (1 << BLINK_LED_PIN);

        // Hold for Loop Period
        while(loop_counter < LOOP_COUNT) {

            // Sleep until Next Interrupt
            cli();
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }

        // Subtract Loop Period for the next loop
        loop_counter = loop_counter - LOOP_COUNT;
    }
}
