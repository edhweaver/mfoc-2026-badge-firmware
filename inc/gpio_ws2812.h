/**
 * @file      gpio_ws2812.h
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

#ifndef GPIO_WS2812_H
#define GPIO_WS2812_H

#include <avr/interrupt.h>
#include <util/delay.h>
#include "pin_config.h"

// Define the colors for the WS2812 LED
enum led_colors {
    COLOR_RED,
    COLOR_BLUE,
    COLOR_GREEN,
    COLOR_WHITE,
    COLOR_BLACK,
};

void ws2812_set_di_pin();
void ws2812_send_byte(uint8_t data);
void ws2812_set_color(uint8_t red, uint8_t green, uint8_t blue);
void ws2812_block_until_safe();

#endif /* GPIO_WS2812_H */