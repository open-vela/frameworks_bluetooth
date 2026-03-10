/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"

#define BTTOOL_STRCAT(dst, size, src, ...)                      \
    do {                                                        \
        size_t _len = strlen(dst);                              \
        size_t _size = (size);                                  \
        if (_len + 1 >= _size)                                  \
            break;                                              \
        snprintf(dst + _len, _size - _len, src, ##__VA_ARGS__); \
    } while (0)

bool phy_is_vaild(uint8_t phy);
int le_addr_type(const char* str, ble_addr_type_t* type);
bool bttool_allocator(void** data, uint32_t size);
uint32_t get_timestamp_msec(void);