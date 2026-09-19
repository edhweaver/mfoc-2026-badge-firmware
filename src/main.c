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

#include "main.h"

//============================================================================
//
//   Phase 3: Diagnostic for WS2812 Addressable LED on PB1 (pin 6)
//
//      Run a diagnostic loop on a WS2812 Addressable LED on PB1 (Pin 6).
//   The diagnostic loop will cycle through the following colors: Red, Blue,
//   Green, White and Black.  Color Change will occur once per loop.
//
//      Blink Led will continue to update once per loop.
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

    // Add State Machine State Variable
    uint8_t state = 0;

    // Add State Machine for WS2812 LED
    enum led_colors next_led_state;

    // Add RGB Color Values for WS2812 LED
    uint8_t led_value_red = 255U;
    uint8_t led_value_green = 0U;
    uint8_t led_value_blue = 0U;

    //initialize Timer0 for 8ms interrupts
    timer0_init();

    // Initialize the WS2812 DI pin
    ws2812_set_di_pin();

    // set processor to sleep mode idle to save power between interrupts
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Initialize the loop counter
    loop_counter = 0;

    // Set Blinking LED as output
    DDRB |= (1 << BLINK_LED_PIN);

    // Set Blinking LED initial state to OFF
    PORTB &= ~(1 << BLINK_LED_PIN);

    // Enable global interrupts
    sei();

    while(1) {

        // Toggle Blinking LED
        PORTB ^= (1 << BLINK_LED_PIN);

        // Set WS2812 LED Color
        ws2812_set_color(led_value_red, led_value_green, led_value_blue);

        // Hold for 2 Timer 0 interrupt (16 milliseconds)
        while(loop_counter < 2) {
            // Sleep until Next Interrupt
            cli();
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }

        // Update LED Color based on state value
        switch(state) {
            case 0:
            case 5:
                next_led_state = COLOR_RED;
                break;
            case 1:
            case 6:
                next_led_state = COLOR_GREEN;
                break;
            case 2:
            case 7:
                next_led_state = COLOR_BLUE;
                break;
            case 3:
            case 8:
                next_led_state = COLOR_WHITE;
                break;
            case 4:
            case 9:
                next_led_state = COLOR_BLACK;
                break;
            default:
                next_led_state = COLOR_RED;
                break;
        }

        // State increments to 9, then back to 0
        if (state < 9) {
            state = state + 1;
        } else {
            state = 0;
        }

        // Sets led color values based on current led state
        // Sets next led state based on current led state
        switch(next_led_state) {
            case COLOR_RED:
                led_value_red = 255U;
                led_value_green = 0U;
                led_value_blue = 0U;
                break;
            case COLOR_GREEN:
                led_value_red = 0U;
                led_value_green = 255U;
                led_value_blue = 0U;
                break;
            case COLOR_BLUE:
                led_value_red = 0U;
                led_value_green = 0U;
                led_value_blue = 255U;
                break;
            case COLOR_WHITE:
                led_value_red = 255U;
                led_value_green = 255U;
                led_value_blue = 255U;
                break;
            default:
            case COLOR_BLACK:
                led_value_red = 0U;
                led_value_green = 0U;
                led_value_blue = 0U;
                break;
        }

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
