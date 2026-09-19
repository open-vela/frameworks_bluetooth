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
#include <errno.h>
#include <nuttx/crc16.h>
#include <pthread.h>
#include <stdint.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "bluetooth_define.h"
#include "service_loop.h"
#include "storage.h"
#include "utils/log.h"
#include "uv_ext.h"

#define BT_DB_FOLDER_PATH "/data/misc/bt"
#define BT_DB_FILE_NAME "bt_storage.db"
#define BT_DB_FILE_PATH BT_DB_FOLDER_PATH "/" BT_DB_FILE_NAME
#define REMOTE_DEVICE_FILE_NAME "device.db"

#define BT_KEY_ADAPTER_INFO "AdapterInfo"
#define BT_KEY_BTBOND "BtBonded"
#define BT_KEY_BLEBOND "BleBonded"
#define BT_KEY_BLEWHITELIST "WhiteList"
#define BT_KEY_BLERESOLVINGLIST "ResolvingList"
#define BT_KEY_BLEGATTHASH "BleGattDBHash"

typedef struct {
    uint16_t items;
    uint16_t key_length;
    uint8_t key_value[0];
} key_header_t;

static uv_db_t* storage_handle = NULL;

/* Every entry point below is reachable from more than one thread: the adapter
 * and profile state machines run on the service loop, while the stack
 * callbacks that load the adapter properties and the bonded devices run on the
 * Bluetooth stack thread. The unqlite handle behind uv_db is shared, so each
 * operation is serialised here and executed synchronously on the calling
 * thread.
 *
 * The callback flavours of uv_db_set()/uv_db_get() are deliberately not used:
 * they hand the work to the libuv thread pool, which means the uv_db request
 * queue is manipulated from whichever thread happens to call in (it is not
 * thread safe), the caller's buffer has to stay alive until an unrelated
 * thread is done with it, and the commit ends up running on a third thread. A
 * corrupted unqlite pager was the visible symptom - see docs_ble. */
static pthread_mutex_t storage_lock = PTHREAD_MUTEX_INITIALIZER;

static void key_get_callback(int status, const char* key, uv_buf_t value, void* cookie)
{
    load_storage_callback_t callback = (load_storage_callback_t)cookie;
    if (status == 0) {
        key_header_t* header = (key_header_t*)value.base;
        assert(value.len == (sizeof(key_header_t) + header->key_length));
        callback(header->key_value, header->key_length, header->items);
    } else
        callback(NULL, 0, 0);
}

/* The buffer belongs to the caller once this returns, whatever the result. */
static int storage_set_key(const char* key, void* data, uint16_t length)
{
    uv_buf_t buf = uv_buf_init((char*)data, length);
    int ret;

    if (!storage_handle)
        return -ENODEV;

    pthread_mutex_lock(&storage_lock);
    ret = uv_db_set(storage_handle, key, &buf, NULL, NULL);
    if (ret == 0)
        ret = uv_db_commit(storage_handle);
    pthread_mutex_unlock(&storage_lock);

    if (ret != 0)
        BT_LOGE("key %s set error:%d", key, ret);

    syslog(ret == 0 ? LOG_INFO : LOG_ERR, "[bt_storage] set %s len=%u ret=%d\n",
        key, length, ret);

    return ret;
}

/* With data != NULL the stored blob is handed back to the caller, which owns
 * it. With data == NULL the blob is passed to the load callback in cookie and
 * released here. A negative return means nothing was read, and the caller is
 * the one that reports that to its own callback - same contract as before. */
static int storage_get_key(const char* key, void** data, uint16_t* length, void* cookie)
{
    uv_buf_t buf = uv_buf_init(NULL, 0);
    int ret;

    if (!storage_handle)
        return -ENODEV;

    pthread_mutex_lock(&storage_lock);
    ret = uv_db_get(storage_handle, key, &buf, NULL, NULL);
    pthread_mutex_unlock(&storage_lock);

    syslog(LOG_INFO, "[bt_storage] get %s ret=%d len=%u\n", key, ret,
        ret == 0 ? (unsigned)buf.len : 0u);

    if (ret != 0)
        return ret;

    if (data) {
        *data = buf.base;
        *length = buf.len;
        return 0;
    }

    /* Outside the lock: the load callbacks re-enter the adapter state machine,
     * which stores keys again. */
    key_get_callback(0, key, buf, cookie);
    free(buf.base);

    return 0;
}

static void adapter_properties_default(adapter_storage_t* prop)
{
    srand(time(NULL));
    int r = rand() % 999;
    snprintf(prop->name, BT_LOC_NAME_MAX_LEN, "%s-%03X", "XIAOMI VELA", r);
    prop->class_of_device = DEFAULT_DEVICE_OF_CLASS;
    prop->io_capability = DEFAULT_IO_CAPABILITY;
    prop->scan_mode = DEFAULT_SCAN_MODE;
    prop->bondable = DEFAULT_BONDABLE_MODE;
}

int bt_storage_save_adapter_info(adapter_storage_t* adapter)
{
    uint16_t length = sizeof(key_header_t) + sizeof(*adapter);
    key_header_t* key = malloc(length);
    int ret;

    if (!key)
        return -ENOMEM;

    key->items = 1;
    key->key_length = sizeof(*adapter);
    memcpy(key->key_value, adapter, sizeof(*adapter));
    ret = storage_set_key(BT_KEY_ADAPTER_INFO, key, length);
    free(key);

    return ret;
}

int bt_storage_load_adapter_info(adapter_storage_t* adapter)
{
    uint16_t len = sizeof(key_header_t) + sizeof(*adapter);
    key_header_t* key;

    if (storage_get_key(BT_KEY_ADAPTER_INFO, (void**)&key, &len, NULL) == 0) {
        memcpy(adapter, key->key_value, sizeof(*adapter));
        free(key);
    } else {
        adapter_properties_default(adapter);
        bt_storage_save_adapter_info(adapter);
    }

    return 0;
}

static int bt_storage_save_remote_device(const char* key, void* value, uint16_t value_size, uint16_t items)
{
    uint16_t total_length = value_size * items;
    uint16_t length = sizeof(key_header_t) + total_length;
    key_header_t* header = malloc(length);
    int ret;

    if (!header)
        return -ENOMEM;

    header->items = items;
    header->key_length = total_length;
    if (value && items)
        memcpy(header->key_value, value, total_length);

    ret = storage_set_key(key, header, length);
    free(header);

    return ret;
}

int bt_storage_save_gatt_cache_device(remote_device_gatt_properties_t* remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BLEGATTHASH, remote, sizeof(*remote), size);
}

int bt_storage_save_bonded_device(remote_device_properties_t* remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BTBOND, remote, sizeof(*remote), size);
}

int bt_storage_save_whitelist(remote_device_le_properties_t* remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BLEWHITELIST, remote, sizeof(*remote), size);
}

int bt_storage_save_le_bonded_device(remote_device_le_properties_t* remote, uint16_t size)
{
    return bt_storage_save_remote_device(BT_KEY_BLEBOND, remote, sizeof(*remote), size);
}

int bt_storage_load_gatt_cache_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BLEGATTHASH, NULL, NULL, (void*)cb);
}

int bt_storage_load_bonded_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BTBOND, NULL, NULL, (void*)cb);
}

int bt_storage_load_whitelist_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BLEWHITELIST, NULL, NULL, (void*)cb);
}

int bt_storage_load_le_bonded_device(load_storage_callback_t cb)
{
    return storage_get_key(BT_KEY_BLEBOND, NULL, NULL, (void*)cb);
}

void bt_storage_load_le_device_info(void)
{
}

void bt_storage_load_irk_info(void)
{
}

int bt_storage_init(void)
{
    int ret;

    ret = uv_db_init(get_service_uv_loop(), &storage_handle, BT_DB_FILE_PATH);
    if (ret != 0) {
        BT_LOGE("%s fail, ret:%d", __func__, ret);
        syslog(LOG_ERR, "[bt_storage] open %s failed: %d\n", BT_DB_FILE_PATH, ret);
        storage_handle = NULL;
        return ret;
    }

    BT_LOGD("%s successed", __func__);
    syslog(LOG_INFO, "[bt_storage] %s ready (synchronous)\n", BT_DB_FILE_PATH);

    return ret;
}

int bt_storage_cleanup(void)
{
    uv_db_t* handle;

    BT_LOGD("%s", __func__);

    pthread_mutex_lock(&storage_lock);
    handle = storage_handle;
    storage_handle = NULL;
    pthread_mutex_unlock(&storage_lock);

    if (handle)
        uv_db_close(handle);

    return 0;
}