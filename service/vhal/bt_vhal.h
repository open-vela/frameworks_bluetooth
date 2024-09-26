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
#ifndef _BT_VHAL_H__
#define _BT_VHAL_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct bt_vhal {
    size_t size;

    int (*open)(int fd);
    int (*send)(uint8_t* value, uint32_t size);
    int (*recv)(uint8_t* value, uint32_t size);
    int (*close)(int fd);
} bt_vhal_interface;

const void* get_bt_vhal_interface();

#endif /* _BT_VHAL_H__ */
