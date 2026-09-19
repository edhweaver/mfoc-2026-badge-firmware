/**
 * @file      gpio_ws2812.c
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     WS2812 LED Driver
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

#include "gpio_ws2812.h"

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

__attribute__((weak)) void ws2812_block_until_safe() {
    ;
}

/* Initialize the WS2812 DI pin */
void ws2812_set_di_pin() {
    DDRB |= (1 << WS2812_DIO_PIN);
}

/* Send a byte of data to the WS2812 DI Pin.  Tuned for an MCU Clock of
   8MHz */
void ws2812_send_byte(uint8_t data) {

    // Send 8 bits
    for (uint8_t counter = 8; counter > 0; counter--) {

        // Send MSB of data
        if (data & 0x80) {
            // Send a '1'
            asm volatile (
                "sbi %0, %1 \n\t" // 2 cycles - Pin HIGH
                "nop        \n\t" // 1 cycle
                "nop        \n\t" // 1 cycle
                "nop        \n\t" // 1 cycle
                "nop        \n\t" // 1 cycle
                "cbi %0, %1 \n\t" // 2 cycles - Pin LOW
                :: "I" (_SFR_IO_ADDR(PORTB)), "I" (WS2812_DIO_PIN)
            );
        } else {
            // Send a '0'
            asm volatile (
                "sbi %0, %1 \n\t" // 2 cycles - Pin HIGH
                "nop        \n\t" // 1 cycle
                "cbi %0, %1 \n\t" // 2 cycles - Pin LOW
                "nop        \n\t" // 1 cycle
                "nop        \n\t" // 1 cycle
                :: "I" (_SFR_IO_ADDR(PORTB)), "I" (WS2812_DIO_PIN)
            );
        }

        // Shift to the next bit
        data <<= 1;
    }
}

/* Send 3 bytes of data to the WS2812 LED. */
void ws2812_set_color(uint8_t red, uint8_t green, uint8_t blue) {

    // block until all time critial actions are completed
    ws2812_block_until_safe();

    // Disable the Global Interrupt Enable bit
    // This mimics the behavior of an Interrupt Service Routine (ISR) on the
    // ATTiny85 MCU.  This ensures that the timing of the WS2812 data signal
    // is not disrupted by interrupts.
    cli();

    // WS2812 uses GRB order
    ws2812_send_byte(red);
    ws2812_send_byte(green);
    ws2812_send_byte(blue);

    // Re-enable the Global Interrupt Enable bit
    sei();

    // Ensure WS2812 DI line is not update for at least 50 msecs
    _delay_us(50);
}
