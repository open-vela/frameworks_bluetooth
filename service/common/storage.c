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

#define LOG_TAG "storage"
#include <nuttx/crc16.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include "bluetooth_define.h"
#include "service_loop.h"
#include "storage.h"
#include "utils/log.h"
#include "uv_ext.h"

<<<<<<< HEAD
#define BT_DB_FOLDER_PATH "/data/misc/bt"
#define BT_DB_FILE_NAME "bt_storage.db"
#define BT_DB_FILE_PATH BT_DB_FOLDER_PATH "/" BT_DB_FILE_NAME
#define REMOTE_DEVICE_FILE_NAME "device.db"

#define BT_KEY_ADAPTER_INFO "AdapterInfo"
#define BT_KEY_BTBOND "BtBonded"
#define BT_KEY_BLEBOND "BleBonded"
#define BT_KEY_BLEWHITELIST "WhiteList"
=======
#define MISC_PATH               "/data/misc"
#define BT_DB_FOLDER_PATH       "/data/misc/bt"
#define BT_DB_FILE_NAME         "bt_storage.db"
#define BT_DB_FILE_PATH         BT_DB_FOLDER_PATH "/" BT_DB_FILE_NAME
#define REMOTE_DEVICE_FILE_NAME "device.db"

#define BT_KEY_ADAPTER_INFO     "AdapterInfo"
#define BT_KEY_BTBOND           "BtBonded"
#define BT_KEY_BLEBOND          "BleBonded"
#define BT_KEY_BLEWHITELIST     "WhiteList"
>>>>>>> bluetooth framework re-implement base
#define BT_KEY_BLERESOLVINGLIST "ResolvingList"

typedef struct {
    uint16_t items;
    uint16_t key_length;
    uint8_t key_value[0];
} key_header_t;

static uv_db_t *storage_handle = NULL;

static void key_set_callback(int status, const char *key, uv_buf_t value, void *cookie)
{
    free(value.base);
    if (status == 0)
        uv_db_commit(storage_handle);
}

static void key_get_callback(int status, const char *key, uv_buf_t value, void *cookie)
{
    load_storage_callback_t callback = (load_storage_callback_t)cookie;
    if (status == 0) {
        key_header_t *header = (key_header_t *)value.base;
        assert(value.len == (sizeof(key_header_t) + header->key_length));
        callback(header->key_value, header->key_length, header->items);
    } else
        callback(NULL, 0, 0);
}

static int storage_set_key(const char *key, void *data, uint16_t length)
{
    uv_buf_t buf = uv_buf_init((char *)data, length);
    int ret = uv_db_set(storage_handle, key, &buf, key_set_callback, NULL);
    if (ret != 0)
        BT_LOGE("key %s set error:%d", key, ret);

    return ret;
}

static int storage_get_key(const char *key, void **data, uint16_t *length, void *cookie)
{
    uv_buf_t buf;

    if (data)
        buf = uv_buf_init(NULL, 0);

    int ret = uv_db_get(storage_handle, key, data ? &buf : NULL,
                        data ? NULL : key_get_callback, cookie);
    if (ret == 0 && data) {
        *data = buf.base;
        *length = buf.len;
    }

    return ret;
}

static void adapter_properties_default(adapter_storage_t *prop)
{
    srand(time(NULL));
    int r = rand() % 999;
    snprintf(prop->name, BT_LOC_NAME_MAX_LEN, "%s-%03X", "XIAOMI VELA", r);
    prop->class_of_device = DEFAULT_DEVICE_OF_CLASS;
    prop->io_capability = DEFAULT_IO_CAPABILITY;
    prop->scan_mode = DEFAULT_SCAN_MODE;
    prop->bondable = DEFAULT_BONDABLE_MODE;
}

int bt_storage_save_adapter_info(adapter_storage_t *adapter)
{
    key_header_t *key = malloc(sizeof(key_header_t) + sizeof(*adapter));

    key->items = 1;
    key->key_length = sizeof(*adapter);
    memcpy(key->key_value, adapter, sizeof(*adapter));
    int ret = storage_set_key(BT_KEY_ADAPTER_INFO, key, sizeof(key_header_t) + sizeof(*adapter));
    if (ret != 0)
        free(key);

    return ret;
}

int bt_storage_load_adapter_info(adapter_storage_t *adapter)
{
    uint16_t len = sizeof(key_header_t) + sizeof(*adapter);
    key_header_t *key;

    if (storage_get_key(BT_KEY_ADAPTER_INFO, (void **)&key, &len, NULL) == 0) {
        memcpy(adapter, key->key_value, sizeof(*adapter));
        free(key);
    } else {
        adapter_properties_default(adapter);
        bt_storage_save_adapter_info(adapter);
    }

    return 0;
}

<<<<<<< HEAD
static int bt_storage_save_remote_device(const char *key, void *value, uint16_t value_size, uint16_t items)
{
    uint16_t total_length = value_size * items;
    key_header_t *header = malloc(sizeof(key_header_t) + total_length);

    header->items = items;
    header->key_length = total_length;
    if (value && items)
        memcpy(header->key_value, value, total_length);

    int ret = storage_set_key(key, header, sizeof(key_header_t) + total_length);
    if (ret != 0)
        free(header);
=======
int bt_storage_save_bonded_device(remote_device_properties_t *remote, uint16_t size)
{
    uint16_t remote_length = sizeof(*remote) * size;
    key_header_t *key = malloc(sizeof(key_header_t) + remote_length);

    key->items = size;
    key->key_length = remote_length;
    if (remote && size)
        memcpy(key->key_value, remote, remote_length);
    int ret = storage_set_key(BT_KEY_BTBOND, key, sizeof(key_header_t) + remote_length);
    if (ret != 0)
        free(key);
>>>>>>> bluetooth framework re-implement base

    return ret;
}

<<<<<<< HEAD
int bt_storage_save_bonded_device(remote_device_properties_t *remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BTBOND, remote, sizeof(*remote), size);
}

int bt_storage_save_whitelist(remote_device_le_properties_t *remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BLEWHITELIST, remote, sizeof(*remote), size);
}

int bt_storage_save_le_bonded_device(remote_device_le_properties_t *remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BLEBOND, remote, sizeof(*remote), size);
}

=======
>>>>>>> bluetooth framework re-implement base
int bt_storage_load_bonded_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BTBOND, NULL, NULL, (void *)cb);
}

<<<<<<< HEAD
int bt_storage_load_whitelist_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BLEWHITELIST, NULL, NULL, (void *)cb);
}

int bt_storage_load_le_bonded_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BLEBOND, NULL, NULL, (void *)cb);
}

=======
>>>>>>> bluetooth framework re-implement base
void bt_storage_load_le_device_info(void)
{
}

void bt_storage_load_irk_info(void)
{
}

int bt_storage_init(void)
{
    int ret;

<<<<<<< HEAD
    ret = uv_db_init(get_service_uv_loop(), &storage_handle, BT_DB_FILE_PATH);
    if (ret != 0)
        BT_LOGE("%s fail, ret:%d", __func__, ret);
=======
    if ((ret = access(MISC_PATH, 0)) && ret != 0) {
        if ((ret = mkdir(MISC_PATH, 0777)) && ret != 0) {
            BT_LOGD("misc folder create fail:%d", ret);
            return ret;
        }
    }

    if ((ret = access(BT_DB_FOLDER_PATH, 0)) && ret != 0) {
        if ((ret = mkdir(BT_DB_FOLDER_PATH, 0777)) && ret != 0) {
            BT_LOGD("storage folder create fail:%d", ret);
            return ret;
        }
    }

    ret = uv_db_init(get_service_uv_loop(), &storage_handle, BT_DB_FILE_PATH);
    if (ret != 0)
        BT_LOGD("%s fail, ret:%d", __func__, ret);
>>>>>>> bluetooth framework re-implement base

    BT_LOGD("%s successed", __func__);

    return ret;
}

int bt_storage_cleanup(void)
{
    BT_LOGD("%s", __func__);
    if (storage_handle)
        uv_db_close(storage_handle);

<<<<<<< HEAD
    storage_handle = NULL;
=======
>>>>>>> bluetooth framework re-implement base
    return 0;
}