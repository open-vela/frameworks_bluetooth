/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef __STORAGE_VERSION_4_H__
#define __STORAGE_VERSION_4_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "storage_update.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* rel-4.0.0 storage structure */
#define BT_NAME_MAX_LEN_4_0_0 63

/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef struct {
    uint16_t items;
    uint16_t key_length;
    uint8_t key_value[0];
} key_header_t; // TODO: remove

typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_remote_device for reasons.
    char name[BT_NAME_MAX_LEN_4_0_0 + 1];
    char alias[BT_NAME_MAX_LEN_4_0_0 + 1];
    uint32_t class_of_device;
    uint8_t link_key[16];
    bt_link_key_type_t link_key_type;
    bt_device_type_t device_type;
} remote_device_properties_v4_0_0_t;

typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_le_remote_device for reasons.
    uint8_t smp_key[80];
    bt_device_type_t device_type;
} remote_device_le_properties_v4_0_0_t;

typedef struct {
    char name[BT_NAME_MAX_LEN_4_0_0 + 1];
    uint32_t class_of_device;
    uint32_t io_capability;
    uint32_t scan_mode;
    uint32_t bondable;
} adapter_storage_v4_0_0_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/* Get diefferent version storage Info */
bt_storage_update_properties_t* bt_storage_load_info_unqlite(void);
bt_storage_update_properties_t* bt_storage_load_info_v4_0_0(void);

#endif /* __STORAGE_VERSION_4_H__ */