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
#ifndef __BLUETOOTH_DEFINE_H_
#define __BLUETOOTH_DEFINE_H_
#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_config.h"
#include "bt_uuid.h"
// #define BLE_MAX_ADV_NUM 8

#define SMP_KEYS_MAX_SIZE 80
#define BT_COMMON_KEY_LENGTH 16

#ifdef CONFIG_BLUETOOTH_DEFAULT_COD
#define DEFAULT_DEVICE_OF_CLASS CONFIG_BLUETOOTH_DEFAULT_COD
#else
#define DEFAULT_DEVICE_OF_CLASS 0x00280704
#endif

#define DEFAULT_IO_CAPABILITY BT_IO_CAPABILITY_NOINPUTNOOUTPUT
#define DEFAULT_SCAN_MODE BT_BR_SCAN_MODE_CONNECTABLE
#define DEFAULT_BONDABLE_MODE 1

typedef enum {
    BT_LINKKEY_TYPE_COMBINATION_KEY,
    BT_LINKKEY_TYPE_LOCAL_UNIT_KEY,
    BT_LINKKEY_TYPE_REMOTE_UNIT_KEY,
    BT_LINKKEY_TYPE_DEBUG_COMBINATION_KEY,
    BT_LINKKEY_TYPE_UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    BT_LINKKEY_TYPE_AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192,
    BT_LINKKEY_TYPE_CHANGED_COMBINATION_KEY,
    BT_LINKKEY_TYPE_UNAUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256,
    BT_LINKKEY_TYPE_AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256
} bt_link_key_type_t;

typedef enum {
    BT_DBG_TYPE_HCI = 1,
    BT_DBG_TYPE_HCI_RAW,
    BT_DBG_TYPE_HCI_DUMP,
    BT_DBG_TYPE_L2CAP,
    BT_DBG_TYPE_SDP,
    BT_DBG_TYPE_ATT,
    BT_DBG_TYPE_SMP,
    BT_DBG_TYPE_RFCOMM,
    BT_DBG_TYPE_OBEX,
    BT_DBG_TYPE_AVCTP,
    BT_DBG_TYPE_AVDTP,
    BT_DBG_TYPE_AVRCP,
    BT_DBG_TYPE_A2DP,
    BT_DBG_TYPE_HFP,
    BT_DBG_TYPE_MAX
} bt_debug_type_t;

typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_remote_device for reasons.
    char name[BT_REM_NAME_MAX_LEN + 1];
    char alias[BT_REM_NAME_MAX_LEN + 1];
    uint32_t class_of_device;
    uint8_t link_key[16];
    bt_link_key_type_t link_key_type;
    bt_device_type_t device_type;
    uint8_t uuids[CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN];
} remote_device_properties_t;

typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    // only can add member after "addr_type" if needed, see function bt_storage_save_le_remote_device for reasons.
    uint8_t smp_key[80];
    bt_device_type_t device_type;
} remote_device_le_properties_t;

typedef struct {
    char name[BT_LOC_NAME_MAX_LEN + 1];
    uint32_t class_of_device;
    uint32_t io_capability;
    uint32_t scan_mode;
    uint32_t bondable;
} adapter_storage_t;

#endif /* __BLUETOOTH_DEFINE_H_ */