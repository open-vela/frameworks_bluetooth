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
#ifndef __BT_SCAN_RECORD_H__
#define __BT_SCAN_RECORD_H__

#include <stdbool.h>
#include <stdint.h>

#define BT_EIR_SVC_DATA_16 0x16 // 16-bit UUID

typedef struct {
    bool active;
    uint16_t uuid;
    int8_t tx_power;
    uint8_t flag;

    uint8_t* name;
    uint8_t name_size;
} scan_record_t;

void scan_record_parse(scan_record_t* record, const uint8_t* eir_data, uint8_t eir_len);

#endif /* __BT_SCAN_RECORD_H__ */
