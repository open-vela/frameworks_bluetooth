/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "index_allocator.h"

index_allocator_t* index_allocator_create(int max)
{
    uint8_t num_index = (max / 32) + 1;
    int size = sizeof(index_allocator_t) + num_index * 4;

    index_allocator_t* allocator = malloc(size);
    if (!allocator)
        return NULL;

    memset(allocator, 0, size);
    allocator->id_max = max;
    allocator->id_next = 0;

    return allocator;
}

void index_allocator_delete(index_allocator_t** allocator)
{
    if (*allocator)
        free(*allocator);

    *allocator = NULL;
}

int index_alloc(index_allocator_t* allocator)
{
    uint8_t start = allocator->id_next;
    uint8_t minor;
    int index;
    int bitno;

    do {
        minor = allocator->id_next;
        if (allocator->id_next >= allocator->id_max)
            allocator->id_next = 0;
        else
            allocator->id_next++;

        index = minor >> 5;
        bitno = minor & 31;
        if ((allocator->id_map[index] & (1 << bitno)) == 0) {
            allocator->id_map[index] |= (1 << bitno);
            return (int)minor;
        }
    } while (allocator->id_next != start);

    return -1;
}

void index_free(index_allocator_t* allocator, uint16_t id)
{
    int index;
    int bitno;

    index = id >> 5;
    bitno = id & 31;

    assert((allocator->id_map[index] & (1 << bitno)) != 0);
    allocator->id_map[index] &= ~(1 << bitno);
    if (id < allocator->id_next)
        allocator->id_next = id;
}