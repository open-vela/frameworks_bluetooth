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

#define EIR_FLAGS 0x01 /* flags */
#define EIR_UUID16_SOME 0x02 /* 16-bit UUID, more available */
#define EIR_UUID16_ALL 0x03 /* 16-bit UUID, all listed */
#define EIR_UUID32_SOME 0x04 /* 32-bit UUID, more available */
#define EIR_UUID32_ALL 0x05 /* 32-bit UUID, all listed */
#define EIR_UUID128_SOME 0x06 /* 128-bit UUID, more available */
#define EIR_UUID128_ALL 0x07 /* 128-bit UUID, all listed */
#define EIR_NAME_SHORT 0x08 /* shortened local name */
#define EIR_NAME_COMPLETE 0x09 /* complete local name */
#define EIR_TX_POWER 0x0A /* transmit power level */
#define EIR_CLASS_OF_DEV 0x0D /* Class of Device */
#define EIR_SSP_HASH 0x0E /* SSP Hash */
#define EIR_SSP_RANDOMIZER 0x0F /* SSP Randomizer */
#define EIR_DEVICE_ID 0x10 /* device ID */
#define EIR_SOLICIT16 0x14 /* LE: Solicit UUIDs, 16-bit */
#define EIR_SOLICIT128 0x15 /* LE: Solicit UUIDs, 128-bit */
#define EIR_SVC_DATA16 0x16 /* LE: Service data, 16-bit UUID */
#define EIR_PUB_TRGT_ADDR 0x17 /* LE: Public Target Address */
#define EIR_RND_TRGT_ADDR 0x18 /* LE: Random Target Address */
#define EIR_GAP_APPEARANCE 0x19 /* GAP appearance */
#define EIR_SOLICIT32 0x1F /* LE: Solicit UUIDs, 32-bit */
#define EIR_SVC_DATA32 0x20 /* LE: Service data, 32-bit UUID */
#define EIR_SVC_DATA128 0x21 /* LE: Service data, 128-bit UUID */
#define EIR_TRANSPORT_DISCOVERY 0x26 /* Transport Discovery Service */
#define EIR_CSIP_RSI 0x2e /* Resolvable Set Identifier */
#define EIR_MANUFACTURER_DATA 0xFF /* Manufacturer Specific Data */

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
