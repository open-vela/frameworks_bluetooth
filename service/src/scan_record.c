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
#define LOG_TAG "scan_record"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bt_utils.h"
#include "scan_record.h"

static void record_parse_uuid16(scan_record_t* record, const uint8_t* data,
    uint8_t len)
{
    STREAM_TO_UINT16(record->uuid, data);
}

void scan_record_parse(scan_record_t* record, const uint8_t* eir_data, uint8_t eir_len)
{
    uint16_t len = 0;

    if (!eir_data) {
        return;
    }

    while (len < eir_len - 1) {
        uint8_t field_len;
        const uint8_t* data;
        uint8_t data_len;

        field_len = eir_data[0];
        if (field_len == 0) {
            break;
        }

        len += field_len + 1;

        if (len > eir_len) {
            break;
        }

        data = &eir_data[2];
        data_len = field_len - 1;

        switch (eir_data[1]) {
        case BT_EIR_SVC_DATA_16:
            record_parse_uuid16(record, data, data_len);
            break;

        /* TODO: handle other eir data */
        default:
            break;
        }

        eir_data += field_len + 1;
    }
}
