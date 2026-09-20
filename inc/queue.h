/**
 * @file      queue.h
 * @author    Ed Weaver <Ed.H.Weaver@gmail.com>
 * @brief     Queue Driver
 * @version   1.0
 * @date      2021-06-01
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

#ifndef QUEUE_H
#define	QUEUE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdint.h>

// Need to include one of the below defines in the compiler options for code
//   to properly build
//#define QUEUE_TINY  1
#define QUEUE_FULL  1

#if (QUEUE_FULL)
typedef struct
{
    volatile uint8_t first;
    volatile uint8_t last;
    volatile uint8_t size;
    volatile uint8_t length;
    volatile uint8_t error_counter;
    volatile uint8_t last_error;
    volatile uint8_t * buffer;
}queue_t;
#else
typedef struct
{
    volatile uint8_t first;
    volatile uint8_t last;
    volatile uint8_t size;
    volatile uint8_t length;
    volatile uint8_t * buffer;
}queue_t;
#endif

#define QUEUE_EMPTY                                      -3
#define QUEUE_OVERFLOW                                   -2
#define QUEUE_MEMORY_LEAK                                -2
#define QUEUE_UNINITIALIZED                              -1
#define QUEUE_SUCCESS                                     0
#define QUEUE_SUCCESS_NO_MATCH                            1

#define QUEUE_ERROR_COUNTER_MAX                       0xFEU

#define QUEUE_CANARY                                    '*'

int8_t Queue_Initialize(queue_t * queue, uint8_t * buffer, uint8_t size);
int8_t Queue_Length(queue_t * queue, uint8_t * length);
int8_t Queue_Inject(queue_t * queue, uint8_t character);
int8_t Queue_Eject(queue_t * queue, uint8_t * character);

#ifdef	__cplusplus
}
#endif

#endif	/* QUEUE_H */
