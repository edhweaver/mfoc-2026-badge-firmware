/**
 * @file      morse.c
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

#include "morse.h"

//============================================================================
//
//   Morse Code Library
//
//      The function morse_convert_to_phrase will convert a buffer of char
//   into a phrase of morse code elements.  There are 4 morse code elements:
//     '.' - One Time Unit Active, One Time Unit Inactive
//     '-' - Three Time Units Active, One Time Unit Inactive
//     '|' - Three Time Units Inactive
//     '_' - Seven Time Units Inactive
//
//      The phrase of morse code elements is null terminated.
//
//      The function morse_parse_phrase will update the states of the morse
//   struct to indicated if the morse code signal is high or low.  There are
//   4 states defined as follows:
//     MORSE_START - New phrase was generated and is ready for parsing
//     MORSE_ACTIVE - Signal is High
//     MORSE_INACTIVE - Signal is Low
//     MORSE_END - Phrase has completed parsing
//
//============================================================================

void morse_init(morse_code_parser_t * state, char * phrase_buffer, uint8_t buffer_size)
{
    state->buffer = phrase_buffer;
    state->buffer_limit = buffer_size;
    state->counter = 0U;
    state->character_index = 0U;
    state->parser_state = MORSE_END;
}

void morse_convert_to_phrase(morse_code_parser_t * state, char * input)
{
    uint8_t index;
    uint8_t element_index;

    // Encode each character of the input as Morse Code Elements
    //  '.' - One Time Unit Active, One Time Unit Inactive
    //  '-' - Three Time Units Active, One Time Unit Inactive
    //  '|' - Three Time Units Inactive
    //  '_' - Seven Time Units Inactive
    //  All other Characters - End of Morse Code Phrase

    // Update States to START if required
    if (state->parser_state == MORSE_END) {
        // Set State to START if parser is at the end of a phrase.
        //   Reset the character index to Zero.
        state->character_index = 0U;
        state->parser_state = MORSE_START;
    } else if (state->parser_state == MORSE_CHARACTER_END) {
        // Set State to START if parser is at the end of a
        //   character.  Increment the character index.
        state->character_index = state->character_index + 1U;
        if (input[state->character_index] == 0x00) {
            state->parser_state = MORSE_END;
        } else {
            state->parser_state = MORSE_START;
        }
    } else {
        // Only process if end of character or end of message
    }

    // Populate Buffer for Morse Code Elements if State is START
    if (state->parser_state == MORSE_START) {

        // Populate Morse Code Elements for 1 character
        element_index = 0U;
        switch (input[state->character_index]) {
            case ' ':
                state->buffer[element_index] = '_';
                element_index = element_index + 1;
                break;
            case '1':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '2':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '3':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '4':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '5':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '6':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '7':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '8':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '9':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '0':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'A':
            case 'a':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'B':
            case 'b':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'C':
            case 'c':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'D':
            case 'd':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'E':
            case 'e':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'F':
            case 'f':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'G':
            case 'g':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'H':
            case 'h':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'I':
            case 'i':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'J':
            case 'j':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'K':
            case 'k':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'L':
            case 'l':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'M':
            case 'm':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'N':
            case 'n':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'O':
            case 'o':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'P':
            case 'p':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'Q':
            case 'q':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'R':
            case 'r':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'S':
            case 's':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'T':
            case 't':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'U':
            case 'u':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'V':
            case 'v':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'W':
            case 'w':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'X':
            case 'x':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'Y':
            case 'y':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case 'Z':
            case 'z':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '.':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '*':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case ',':
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            case '?':
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '-';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '.';
                element_index = element_index + 1;
                state->buffer[element_index] = '|';
                element_index = element_index + 1;
                break;
            default:
                break;
        }

        // Fill Remaining Buffer with Null Characters
        for (index = element_index; index < state->buffer_limit; index++) {
            state->buffer[index] = '\0';
        }
    }
}

void morse_parse_phrase(morse_code_parser_t * state) {

    enum morse_code_states next_state;

    switch(state->parser_state) {
        case MORSE_START:
            // Set next state based on the first Morse Code Element.  Set
            //   the Looping Count to hold in the Next State.  Initialize
            //   the Morse Code Element Index to Zero.
            state->element_index = 0U;
            if (state->buffer[state->element_index] == '.') {
                state->counter = 0U;
                next_state = MORSE_ACTIVE_EDGE;
            } else if (state->buffer[state->element_index] == '-') {
                state->counter = 2U;
                next_state = MORSE_PROCESS_ACTIVE;
            } else if (state->buffer[state->element_index] == '|') {
                state->counter = 2U;
                next_state = MORSE_PROCESS_INACTIVE;
            } else if (state->buffer[state->element_index] == '_') {
                state->counter = 6U;
                next_state = MORSE_PROCESS_INACTIVE;
            } else {
                state->counter = 0U;
                next_state = MORSE_END;
            }
            break;
        case MORSE_PROCESS_ACTIVE:
            // Repeat call to state PROCESS ACTIVE based on value of the
            //   Looping Count.  Set to state ACTIVE EDGE once repeated
            //   is done.
            if (state->counter > 0) {
                state->counter = state->counter - 1U;
                next_state = MORSE_PROCESS_ACTIVE;
            } else {
                next_state = MORSE_ACTIVE_EDGE;
            }
            break;
        case MORSE_ACTIVE_EDGE:
            // Next state is the pause between Morse Code Elements.
            state->counter = 0U;
            next_state = MORSE_INACTIVE_EDGE;
            break;
        case MORSE_PROCESS_INACTIVE:
            // Repeat call to state PROCESS INACTIVE based on value of the
            //   Looping Count.  Set to state INACTIVE EDGE once repeated
            //   is done.
            if (state->counter > 0) {
                state->counter = state->counter - 1U;
                next_state = MORSE_PROCESS_INACTIVE;
            } else {
                next_state = MORSE_INACTIVE_EDGE;
            }
            break;
        case MORSE_INACTIVE_EDGE:
            // Set to an END state if the Morse Code Element is the end of a
            //   phrase.  Otherwise update the Morse Code Element and set to
            //   the Next State based on the new Morse Code Element.
            if (state->buffer[state->element_index] == '|') {
                // Set to sate END due to the Morse Code Element '|'
                state->counter = 0U;
                next_state = MORSE_CHARACTER_END;
            } else if (state->buffer[state->element_index] == '_') {
                // Set to sate END due to the Morse Code Element '_'
                state->counter = 0U;
                next_state = MORSE_CHARACTER_END;
            } else {
                // Update Morse Code Element Index to next Morse Code Element.
                //   Set next state based on the new Morse Code Element.  Set
                //   the Looping Count to hold in the Next State.
                state->element_index = state->element_index + 1;
                if (state->buffer[state->element_index] == '.') {
                    state->counter = 0U;
                    next_state = MORSE_ACTIVE_EDGE;
                } else if (state->buffer[state->element_index] == '-') {
                    state->counter = 2U;
                    next_state = MORSE_PROCESS_ACTIVE;
                } else if (state->buffer[state->element_index] == '|') {
                    state->counter = 2U;
                    next_state = MORSE_PROCESS_INACTIVE;
                } else if (state->buffer[state->element_index] == '_') {
                    state->counter = 6U;
                    next_state = MORSE_PROCESS_INACTIVE;
                } else {
                    state->counter = 0U;
                    next_state = MORSE_END;
                }
            }
            break;
        case MORSE_END:
            next_state = MORSE_END;
            break;
        default:
            next_state = MORSE_CHARACTER_END;
            break;
    }

    state->parser_state = next_state;
}
