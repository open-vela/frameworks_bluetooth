/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#include "bt_hash.h"

#define HASH4 h = ((h << 5) + h + *key++);

uint32_t bt_hash4(const void* keyarg, size_t len)
{
    const uint8_t* key = keyarg;
    uint32_t h = 0;
    size_t loop;

    if (len > 0) {
        loop = (len + 8 - 1) >> 3;
        switch (len & (8 - 1)) {
        case 0:
            do {
                HASH4;
            case 7:
                HASH4;
            case 6:
                HASH4;
            case 5:
                HASH4;
            case 4:
                HASH4;
            case 3:
                HASH4;
            case 2:
                HASH4;
            case 1:
                HASH4;
            } while (--loop);
        }
    }

    return h;
}