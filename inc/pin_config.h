/**
 * @file      pin_config.h
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

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

//============================================================================
//
//                                 ATTINY85
//                            .----------------.
//                  RESET 1 --| PB5        VCC |-- 8
//                        2 --| PB3        PB2 |-- 7
//                  BLINK 3 --| PB4        PB1 |-- 6 WS2812
//                        4 --| GND        PB0 |-- 5
//                            '----------------'
//
//============================================================================
//
//                         Programming Pins ATTINY85
//                            .----------------.
//                  RESET 1 --| PB5        VCC |-- 8
//                        2 --| PB3        PB2 |-- 7 SCK
//                        3 --| PB4        PB1 |-- 6 MISO
//                        4 --| GND        PB0 |-- 5 MOSI
//                            '----------------'
//
//============================================================================
//
//                           Programming Header
//                                 .-----.
//                        MISO 1 --| . . |-- 2 VCC
//                         SCK 3 --| . . |-- 4 MOSI
//                       RESET 5 --| . . |-- 6 GND
//                                 '-----'
//
//============================================================================

// Define the blinking LED pin
#define BLINK_LED_PIN PB4

// Define PB1 as the Data pin for the WS2812 LED
#define WS2812_DIO_PIN PB1

#endif /* PIN_CONFIG_H */