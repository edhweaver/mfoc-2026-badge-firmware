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
//   Phase 7: LED ON/OFF based on Button X Input on PB0 (pin 5)
//
//      Verify that the Button X Input on Pin 5 can read a can be debounced
//   with Timer0 at a 0.4 second debounce time.
//
//      When Button X Input is high, the WS2812 LED on PB1 (pin 6) will
//   blink based on the ADC value.  When Button X Input is low, the WS2812
//   remain off.
//
//============================================================================

// Switch debounce set to 40ms (5 interrupts at 8ms each)
#define SWITCH_DEBOUNCE_COUNT 5

// Button debounce set to 40ms (5 interrupts at 8ms each)
#define BUTTON_DEBOUNCE_COUNT 5

// Loop count set to 200ms (25 interrupts at 8ms each)
#define LOOP_COUNT 25

// Loop counter incremented by Timer 0 interrupt
volatile uint8_t loop_counter = 0;

/* Timer/Counter0 Compare Match A */
ISR(TIMER0_COMPA_vect) {
    static uint8_t switch_y_high_counter = 0;
    static uint8_t switch_y_low_counter = 0;
    static uint8_t button_x_high_counter = 0;
    static uint8_t button_x_low_counter = 0;

    if (loop_counter < 255U) {
        loop_counter++;
    }

    // Clear Switch Y relevant counter based on Switch Y GPIO state 
    if (PINB & (1 << SWITCH_Y_PIN)) {
        // Increment high counter until debounced
        if (switch_y_high_counter <= SWITCH_DEBOUNCE_COUNT) {
            switch_y_high_counter++;
        }
        // Clear low counter
        switch_y_low_counter = 0;
    } else {
        // Clear high counter
        switch_y_high_counter = 0;
        // Increment low counter until debounced
        if (switch_y_low_counter <= SWITCH_DEBOUNCE_COUNT) {
            switch_y_low_counter++;
        }
    }

    // Clear Button X relevant counter based on Button X GPIO state
    if (PINB & (1 << BUTTON_X_PIN)) {
        // Clear low counter
        button_x_low_counter = 0;
        // Increment high counter until debounced
        if (button_x_high_counter <= BUTTON_DEBOUNCE_COUNT) {
            button_x_high_counter++;
        }
    } else {
        // Increment low counter until debounced
        if (button_x_low_counter <= BUTTON_DEBOUNCE_COUNT) {
            button_x_low_counter++;
        }
        // Clear high counter
        button_x_high_counter = 0;
    }

    // Edge trigger of debounced switch change
    if (switch_y_high_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__SWITCH_Y_OFF;
    } else {
        CLEAR_FLAG__SWITCH_Y_OFF;
    }

    // Edge trigger of debounced switch change
    if (switch_y_low_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__SWITCH_Y_ON;
    } else {
        CLEAR_FLAG__SWITCH_Y_ON;
    }

    // Edge trigger of debounced button change
    if (button_x_high_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__BUTTON_X_RELEASED;
    } else {
        CLEAR_FLAG__BUTTON_X_RELEASED;
    }

    // Edge trigger of debounced button change
    if (button_x_low_counter >= SWITCH_DEBOUNCE_COUNT) {
        SET_FLAG__BUTTON_X_PRESSED;
    } else {
        CLEAR_FLAG__BUTTON_X_PRESSED;
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

/* Initialize the ADC Input */
void adc_init() {
    // set ADC input pin as input
    DDRB &= ~(1 << ADC_INPUT_PIN);

    // Set ADC input pin, Left adjust ADC, and set Vcc to Vref
    ADMUX = (1 << ADLAR) | ADC_INPUT_CHANNEL;

    // Enable the ADC and set the prescaler to 64 for an ADC clock of 125 kHz
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
}

uint8_t adc_read() {
    // Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion complete
    while (ADCSRA & (1 << ADSC));

    return ADCH; // Return 8-bit value
}

/* Initialize the Switch Y Input */
void switch_y_init() {
    DDRB &= ~(1 << SWITCH_Y_PIN);
}

/* Initialize the Button X Input */
void button_x_init() {
    PORTB |= (1 << BUTTON_X_PIN);
    DDRB &= ~(1 << BUTTON_X_PIN);
}

int main(void) {

    CLEAR_ALL_FLAGS

    // Add State Machine State Variable
    uint8_t state = 0;

    // Add State Machine for WS2812 LED
    enum led_colors next_led_state;

    // Add RGB Color Values for WS2812 LED
    uint8_t led_value_red = 255U;
    uint8_t led_value_green = 0U;
    uint8_t led_value_blue = 0U;

    // Initialize the ADC Input
    uint8_t adc_value = 0U;

    //initialize Timer0 for 8ms interrupts
    timer0_init();

    // Initialize the ADC Input
    adc_init();

    // Initialize the WS2812 DI pin
    ws2812_set_di_pin();

    // Initialize the Switch Y Input
    switch_y_init();

    // Initialize the Button X Input
    button_x_init();

    // set processor to sleep mode idle to save power between interrupts
    set_sleep_mode(SLEEP_MODE_IDLE);

    // Initialize the loop counter
    loop_counter = 0;

    // Enable global interrupts
    sei();

    while(1) {
        // Read the ADC value
        adc_value = adc_read();

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
            case 2:
            case 4:
            case 6:
            case 8:
                if (adc_value < 0x2BU) {
                    next_led_state = COLOR_GREEN;
                } else if (adc_value < 0x6AU) {
                    next_led_state = COLOR_AQUA;
                } else if (adc_value < 0x95U) {
                    next_led_state = COLOR_BLUE;
                } else if (adc_value < 0xD5U) {
                    next_led_state = COLOR_VIOLET;
                } else {
                    next_led_state = COLOR_RED;
                }
                break;
            case 1:
            case 3:
            case 5:
            case 7:
            case 9:
            default:
                next_led_state = COLOR_BLACK;
                break;
        }

        // State increments to 9, then back to 0
        if (state < 9) {
            state = state + 1;
        } else {
            state = 0;
        }

    if (READ_FLAG__BUTTON_X_PRESSED) {
        next_led_state = COLOR_BLACK;
    }

        // Sets led color values based on current led state
        // Sets next led state based on current led state
        switch(next_led_state) {
            case COLOR_VIOLET:
                led_value_red = 255U;
                led_value_green = 0U;
                led_value_blue = 255U;
                break;
            case COLOR_YELLOW:
                led_value_red = 255U;
                led_value_green = 255U;
                led_value_blue = 0U;
                break;
            case COLOR_AQUA:
                led_value_red = 0U;
                led_value_green = 255U;
                led_value_blue = 255U;
                break;
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
