 /**
 * @file      queue.c
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

#include "queue.h"

int8_t Queue_Initialize(queue_t * queue, uint8_t * buffer, uint8_t size)
{
  int8_t return_value;

  if (size > 1U)
  {
    queue->buffer = buffer;
    queue->first = 0U;
    queue->last = 0U;
    queue->length = 0U;
    queue->size = size;
    return_value = QUEUE_SUCCESS;
  }
  else
  {
    return_value = QUEUE_UNINITIALIZED;
  }

  return return_value;
}

int8_t Queue_Length(queue_t * queue, uint8_t * length)
{
  int8_t return_value;

  length[0] = 0U;
  if (queue->size <= 0U)
  {
    return_value = QUEUE_UNINITIALIZED;
  }
  else
  {
    length[0] = queue->length;
    return_value = QUEUE_SUCCESS;
  }

  return return_value;
}

int8_t Queue_Inject(queue_t * queue, uint8_t character)
{
  int8_t return_value;

  if (queue->length < queue->size)
  {
    queue->buffer[queue->last] = character;
    queue->last = queue->last + 1U;
    queue->length = queue->length + 1U;
    if (queue->last >= queue->size)
    {
      queue->last = 0U;
    }
    return_value = QUEUE_SUCCESS;
  }
  else if (queue->size > 1U)
  {
    queue->buffer[queue->last] = character;
    return_value = QUEUE_OVERFLOW;
  }
  else
  {
    return_value = QUEUE_UNINITIALIZED;
  }

  return return_value;
}

int8_t Queue_Eject(queue_t * queue, uint8_t * character)
{
  int8_t return_value;

  if (queue->length > 0U)
  {
    character[0] = queue->buffer[queue->first];
    queue->length = queue->length - 1U;
    queue->first = queue->first + 1U;
    if (queue->first >= queue->size)
    {
      queue->first = 0U;
    }
    return_value = QUEUE_SUCCESS;
  }
  else
  {
    return_value = QUEUE_EMPTY;
  }

  return return_value;
}
