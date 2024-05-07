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
#ifndef __INDEX_ALLOCATOR_H__
#define __INDEX_ALLOCATOR_H__

#include <stdint.h>

typedef struct {
    uint32_t id_max;
    uint8_t id_next;
    uint32_t id_map[0];
} index_allocator_t;

index_allocator_t* index_allocator_create(int max);
void index_allocator_delete(index_allocator_t** allocator);
int index_alloc(index_allocator_t* allocator);
void index_free(index_allocator_t* allocator, uint16_t id);

#endif /* __INDEX_ALLOCATOR_H__ */