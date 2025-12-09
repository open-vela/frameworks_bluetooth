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
#ifndef __STORAGE_VERSION_5_H__
#define __STORAGE_VERSION_5_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "storage_version_4.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define BT_NAME_MAX_LEN_5_X 64

/****************************************************************************
 * Public Types
 ****************************************************************************/
/* v5_0_0 storage structure */
typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_remote_device for reasons.
    char name[BT_NAME_MAX_LEN_5_X + 1];
    char alias[BT_NAME_MAX_LEN_5_X + 1];
    uint32_t class_of_device;
    uint8_t link_key[16];
    bt_link_key_type_t link_key_type;
    bt_device_type_t device_type;
} remote_device_properties_v5_0_0_t;

typedef remote_device_le_properties_v4_0_0_t remote_device_le_properties_v5_0_0_t;

typedef struct {
    char name[BT_NAME_MAX_LEN_5_X + 1];
    uint32_t class_of_device;
    uint32_t io_capability;
    uint32_t scan_mode;
    uint32_t bondable;
} adapter_storage_v5_0_0_t;

/* v5_0_1 storage structure */
typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_remote_device for reasons.
    char name[BT_NAME_MAX_LEN_5_X + 1];
    char alias[BT_NAME_MAX_LEN_5_X + 1];
    uint32_t class_of_device;
    uint8_t link_key[16];
    bt_link_key_type_t link_key_type;
    bt_device_type_t device_type;
    uint8_t uuids[CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN];
} remote_device_properties_v5_0_1_t;

typedef adapter_storage_v5_0_0_t adapter_storage_v5_0_1_t;

typedef remote_device_le_properties_v5_0_0_t remote_device_le_properties_v5_0_1_t;

/* v5_0_2 storage structure */
typedef struct {
    bt_address_t addr;
    uint8_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_remote_device for reasons.
    char name[BT_REM_NAME_MAX_LEN + 1];
    char alias[BT_REM_NAME_MAX_LEN + 1];
    uint8_t link_key_type;
    uint8_t device_type;
    uint8_t pad[1];
    uint8_t link_key[16];
    uint32_t class_of_device;
    uint8_t uuids[CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN];
} __attribute__((aligned(4))) remote_device_properties_v5_0_2_t;

typedef struct {
    bt_address_t addr;
    uint8_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_le_remote_device for reasons.
    uint8_t device_type;
    uint8_t smp_key[80];
} __attribute__((aligned(4))) remote_device_le_properties_v5_0_2_t;

typedef struct {
    char name[BT_LOC_NAME_MAX_LEN + 1];
    uint8_t pad[3];
    uint32_t class_of_device;
    uint32_t io_capability;
    uint32_t scan_mode;
    uint32_t bondable;
} __attribute__((aligned(4))) adapter_storage_v5_0_2_t;

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/* Get diefferent version storage Info */
bt_storage_update_properties_t* bt_storage_load_info_v5_0_0(void);
bt_storage_update_properties_t* bt_storage_load_info_v5_0_1(void);
bt_storage_update_properties_t* bt_storage_load_info_v5_0_2(void);
bt_storage_update_properties_t* bt_storage_load_info_v5_0_3(void);

/* update function */
bt_storage_update_properties_t* bt_storage_update_v4_0_0_to_v5_0_0(bt_storage_update_properties_t* old_storage);
bt_storage_update_properties_t* bt_storage_update_v5_0_0_to_v5_0_1(bt_storage_update_properties_t* old_storage);
bt_storage_update_properties_t* bt_storage_update_v5_0_1_to_v5_0_2(bt_storage_update_properties_t* old_storage);
bt_storage_update_properties_t* bt_storage_update_v5_0_2_to_v5_0_3(bt_storage_update_properties_t* old_storage);

#endif /* __STORAGE_VERSION_5_H__ */