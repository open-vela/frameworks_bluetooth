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

#define LOG_TAG "storage_property"
#include <nuttx/crc16.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "bluetooth_define.h"
#include "service_loop.h"
#include "storage_property.h"
#include "utils/log.h"
#include "uv_ext.h"

#define GET_PROP_KEY(buf, key, address) snprintf((buf), sizeof((buf)), "%s%02X:%02X:%02X:%02X:%02X:%02X", \
    (key),                                                                                                \
    (address)->addr[5], (address)->addr[4], (address)->addr[3],                                           \
    (address)->addr[2], (address)->addr[1], (address)->addr[0]);

#define ERROR_ADAPTERINFO_VALUE -1

#define BT_KVDB_ADAPTERINFO_NAME "persist.bluetooth.adapterInfo.name"
#define BT_KVDB_ADAPTERINFO_COD "persist.bluetooth.adapterInfo.class_of_device"
#define BT_KVDB_ADAPTERINFO_IOCAP "persist.bluetooth.adapterInfo.io_capability"
#define BT_KVDB_ADAPTERINFO_SCAN "persist.bluetooth.adapterInfo.scan_mode"
#define BT_KVDB_ADAPTERINFO_BOND "persist.bluetooth.adapterInfo.bondable"

static void adapter_properties_default(adapter_storage_t* prop)
{
    srand(time(NULL));
    snprintf(prop->name, BT_LOC_NAME_MAX_LEN, "%s-%03X", "XIAOMI VELA", rand() % 999);
    prop->class_of_device = DEFAULT_DEVICE_OF_CLASS;
    prop->io_capability = DEFAULT_IO_CAPABILITY;
    prop->scan_mode = DEFAULT_SCAN_MODE;
    prop->bondable = DEFAULT_BONDABLE_MODE;
}

int bt_storage_save_adapter_info(adapter_storage_t* adapter)
{
    property_set_buffer(BT_KVDB_ADAPTERINFO_NAME, adapter->name, sizeof(adapter->name));
    property_set_int32(BT_KVDB_ADAPTERINFO_COD, adapter->class_of_device);
    property_set_int32(BT_KVDB_ADAPTERINFO_IOCAP, adapter->io_capability);
    property_set_int32(BT_KVDB_ADAPTERINFO_SCAN, adapter->scan_mode);
    property_set_int32(BT_KVDB_ADAPTERINFO_BOND, adapter->bondable);
    property_commit();
    return 0;
}

int bt_storage_load_adapter_info(adapter_storage_t* adapter)
{
    if (property_get_buffer(BT_KVDB_ADAPTERINFO_NAME, adapter->name, sizeof(adapter->name)) > 0) {
        adapter->class_of_device = property_get_int32(BT_KVDB_ADAPTERINFO_COD, ERROR_ADAPTERINFO_VALUE);
        adapter->io_capability = property_get_int32(BT_KVDB_ADAPTERINFO_IOCAP, ERROR_ADAPTERINFO_VALUE);
        adapter->scan_mode = property_get_int32(BT_KVDB_ADAPTERINFO_SCAN, ERROR_ADAPTERINFO_VALUE);
        adapter->bondable = property_get_int32(BT_KVDB_ADAPTERINFO_BOND, ERROR_ADAPTERINFO_VALUE);
    } else {
        adapter_properties_default(adapter);
        bt_storage_save_adapter_info(adapter);
    }

    return 0;
}

int bt_storage_save_bonded_device(remote_device_properties_t* remote, uint16_t size)
{
    return -1;
}

int bt_storage_save_whitelist(remote_device_le_properties_t* remote, uint16_t size)
{
    return -1;
}

int bt_storage_save_le_bonded_device(remote_device_le_properties_t* remote, uint16_t size)
{
    return -1;
}

int bt_storage_load_bonded_device(load_storage_callback_t cb)
{
    return -1;
}

int bt_storage_load_whitelist_device(load_storage_callback_t cb)
{
    return -1;
}

int bt_storage_load_le_bonded_device(load_storage_callback_t cb)
{
    return -1;
}

int bt_storage_delete_bonded_device(bt_address_t* addr)
{
    return -1;
}

int bt_storage_delete_whitelist_device(bt_address_t* addr)
{
    return -1;
}

int bt_storage_delete_le_bonded_device(bt_address_t* addr)
{
    return -1;
}

int bt_storage_init(void)
{
    return -1;
}

int bt_storage_cleanup(void)
{
    return -1;
}