/**
 * @file      morse.h
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     Morse Code Library
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

#ifndef MORSE_H
#define MORSE_H

#include <stdio.h>

// Define states for the NEC Transmitter behavior
enum morse_code_states {
    MORSE_START,
    MORSE_PROCESS_ACTIVE,
    MORSE_ACTIVE_EDGE,
    MORSE_PROCESS_INACTIVE,
    MORSE_INACTIVE_EDGE,
    MORSE_PROCESS_END,
    MORSE_CHARACTER_END,
    MORSE_END
};

typedef struct {
    uint8_t counter;
    enum morse_code_states parser_state;
    char * buffer;
    uint8_t buffer_limit;
    uint8_t character_index;
    uint8_t element_index;
} morse_code_parser_t;

//============================================================================

// Maximum Number of Elements that will encode from a Character
#define MORSE_MAXIMUM_CHARACTERS                                             8

//============================================================================

void morse_init(morse_code_parser_t * state, char * phrase_buffer, uint8_t buffer_size);
void morse_convert_to_phrase(morse_code_parser_t * state, char * input);
void morse_parse_phrase(morse_code_parser_t * state);

#endif /* MORSE_H */