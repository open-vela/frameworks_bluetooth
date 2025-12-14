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
#include <inttypes.h>
#include <kvdb.h>

#include "bluetooth_define.h"
#include "storage.h"
#include "bt_utils.h"
#include "storage_update.h"
#include "storage_version_4.h"
#include "uv_ext.h"

#include "syslog.h"

typedef int (*load_storage_info_unqlite_t)(void** data, uint16_t* length);

const static load_storage_info_unqlite_t storage_items_map[] = {
    bt_storage_load_adapter_info_unqlite,
    bt_storage_load_bonded_device_unqlite,
    bt_storage_load_le_bonded_device_unqlite,
    bt_storage_load_whitelist_device_unqlite
};

bt_storage_update_properties_t* bt_storage_load_info_unqlite(void)
{
    bt_storage_update_properties_t* properties;
    key_header_t* unqlite_value = NULL;
    uint16_t value_length;
    int i;

    properties = zalloc(sizeof(bt_storage_update_properties_t));
    if (!properties) {
        syslog(LOG_ERR, "%s properties malloc failed\n", __func__);
        return NULL;
    }

    /* load storage info */
    for (i = 0; i < ARRAY_SIZE(storage_items_map); ++i) {
        if (storage_items_map[i]((void**)&unqlite_value, &value_length)) {
            syslog(LOG_DEBUG, "%s load storage info[%d] failed\n", __func__, i);
            if (i == BT_STORAGE_UPDATE_ADAPTER_INFO) {
                syslog(LOG_ERR, "%s load adapter info failed\n", __func__);
                goto error;
            }
            continue;
        }

        if (value_length != sizeof(key_header_t) + unqlite_value->key_length) {
            syslog(LOG_ERR, "%s load info[%d], length mismatch([%d] != [%d])\n", __func__, i,
                value_length, sizeof(key_header_t) + unqlite_value->key_length);
            free(unqlite_value);
            goto error;
        }

        properties->storage_info[i].value = zalloc(unqlite_value->key_length);
        if (!properties->storage_info[i].value) {
            syslog(LOG_ERR, "%s storage info[%d] malloc failed\n", __func__, i);
            free(unqlite_value);
            goto error;
        }

        memcpy(properties->storage_info[i].value, unqlite_value->key_value, unqlite_value->key_length);
        properties->storage_info[i].items = unqlite_value->items;
        properties->storage_info[i].value_length = unqlite_value->key_length;
        free(unqlite_value);
    }

    return properties;

error:
    for (i = 0; i < BT_STORAGE_UPDATE_ITEM_MAX; i++) {
        if (properties->storage_info[i].value)
            free(properties->storage_info[i].value);
    }

    free(properties);

    return NULL;
}

bt_storage_update_properties_t* bt_storage_load_info_v4_0_0(void)
{
    return bt_storage_load_info_unqlite();
}
