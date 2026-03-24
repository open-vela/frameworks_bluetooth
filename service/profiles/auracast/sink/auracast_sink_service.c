/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "auracast_sink"
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "auracast_sink_service.h"

#include "auracast_sink_event.h"
#include "auracast_sink_state_machine.h"
#include "bt_list.h"
#include "bt_message_auracast_sink.h"
#include "bt_utils.h"
#include "sal_auracast_sink_interface.h"
#include "service_loop.h"
#include "service_manager.h"

#include "utils/log.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define AURACAST_SINK_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, bt_auracast_sink_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct {
    bt_le_address_t addr; /**< put addr first since it's the most frequently accessed */
    bt_controller_id_t id;
    uint8_t sid;
    auracast_sink_state_machine_t* stm;
} auracast_sink_device_t;

typedef struct {
    bt_list_t* sink_list; /**< auracast_sink_device_t */
    callbacks_list_t* callbacks;
    void* terminating;
} auracast_sink_service_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/
static auracast_sink_service_t g_auracast_sink_service = { 0 };

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static inline auracast_sink_device_t* device_new(bt_controller_id_t id, const bt_le_address_t* addr,
    uint8_t sid)
{
    auracast_sink_device_t* device;

    if (!g_auracast_sink_service.sink_list)
        return NULL;

    device = zalloc(sizeof(auracast_sink_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_le_address_t));
    device->id = id;
    device->sid = sid;
    device->stm = auracast_sink_state_machine_new(device);
    if (!device->stm) {
        free(device);
        return NULL;
    }

    bt_list_add_tail(g_auracast_sink_service.sink_list, device);

    return device;
}

static void device_delete(void* data)
{
    auracast_sink_device_t* device = (auracast_sink_device_t*)data;
    auracast_sink_msg_t* msg = auracast_sink_msg_new(AURACAST_SINK_SYNC_TERMINATED, device->id,
        &device->addr, device->sid);

    if (msg)
        auracast_sink_state_machine_handle_event(device->stm, msg);

    auracast_sink_state_machine_destroy(device->stm);
    free(device);
}

static bool msg_cmp(void* data, void* context)
{
    const auracast_sink_device_t* device = (const auracast_sink_device_t*)data;
    const auracast_sink_msg_t* msg = (const auracast_sink_msg_t*)context;

    if (memcmp(&device->addr, &msg->addr, sizeof(bt_le_address_t)))
        return false;

    if (device->sid != msg->sid)
        return false;

    if (device->id != msg->id)
        return false;

    return true;
}

static inline auracast_sink_device_t* find_device_by_msg(const auracast_sink_msg_t* msg)
{
    if (!g_auracast_sink_service.sink_list)
        return NULL;

    return bt_list_find(g_auracast_sink_service.sink_list, msg_cmp, (void*)msg);
}

static inline auracast_sink_device_t* find_device(bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid)
{
    auracast_sink_msg_t msg = { 0 };
    memcpy(&msg.addr, addr, sizeof(bt_le_address_t));
    msg.id = id;
    msg.sid = sid;

    return find_device_by_msg(&msg);
}

static auracast_sink_device_t* find_or_create_device(bt_controller_id_t id,
    const bt_le_address_t* addr, uint8_t sid)
{
    auracast_sink_device_t* device;
    if ((device = find_device(id, addr, sid)) != NULL)
        return device;

    return device_new(id, addr, sid);
}

static bt_status_t auracast_sink_create_sync(const bt_le_address_t* addr, uint8_t sid,
    uint32_t bitfield, const uint8_t* broadcast_code)
{
    auracast_sink_event_create_sync_t* payload;
    auracast_sink_msg_t* msg = auracast_sink_msg_new_ext(AURACAST_SINK_CREATE_SYNC, PRIMARY_ADAPTER,
        addr, sid, sizeof(auracast_sink_event_create_sync_t));
    if (!msg)
        return BT_STATUS_NOMEM;

    payload = (auracast_sink_event_create_sync_t*)msg->data.data;
    payload->bitfield = bitfield;
    payload->encrypted = broadcast_code != NULL;
    if (broadcast_code)
        memcpy(payload->broadcast_code, broadcast_code, BT_AURACAST_BROADCAST_CODE_LEN);

    auracast_sink_send_message(msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t auracast_sink_terminate_sync(const bt_le_address_t* addr, uint8_t sid)
{
    auracast_sink_send_message(auracast_sink_msg_new(AURACAST_SINK_TERMINATE_SYNC, PRIMARY_ADAPTER,
        addr, sid));

    return BT_STATUS_SUCCESS;
}

static void sink_cleanup(void* data, void* context)
{
    auracast_sink_device_t* device = (auracast_sink_device_t*)data;

    auracast_sink_terminate_sync(&device->addr, device->sid);
}

static void service_startup(const auracast_sink_msg_t* msg)
{
    auracast_sink_service_t* service = &g_auracast_sink_service;
    profile_on_startup_t cb = (profile_on_startup_t)msg->context;

    service->sink_list = bt_list_new(device_delete);
    if (!service->sink_list)
        goto error;

    service->callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    if (!service->callbacks)
        goto error;

    if (bt_sal_auracast_sink_init() != BT_STATUS_SUCCESS)
        goto error;

    cb(PROFILE_AURACAST_SINK, true);
    return;

error:
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    bt_list_free(service->sink_list);
    service->sink_list = NULL;

    cb(PROFILE_AURACAST_SINK, false);
}

static void service_shutdown(const auracast_sink_msg_t* msg)
{
    auracast_sink_service_t* service = &g_auracast_sink_service;
    profile_on_shutdown_t cb = (profile_on_shutdown_t)msg->context;

    BT_LOGD("%s", __func__);

    service->terminating = cb;
    if (bt_list_length(service->sink_list) > 0) {
        bt_list_foreach(service->sink_list, sink_cleanup, NULL);
        BT_LOGD("%s, wait for sink disconnected", __func__);
        return; /**< wait for sink disconnected */
    }

    bt_sal_auracast_sink_cleanup();

    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;

    bt_list_free(service->sink_list);
    service->sink_list = NULL;
    service->terminating = NULL;

    cb(PROFILE_AURACAST_SINK, true);
    return;
}

static void auracast_sink_process_message(void* data)
{
    auracast_sink_msg_t* msg = (auracast_sink_msg_t*)data;
    auracast_sink_device_t* device = NULL;

    if (!msg)
        return;

    switch (msg->event) {
    case AURACAST_SINK_STARTUP:
        service_startup(msg);
        break;
    case AURACAST_SINK_SHUTDOWN:
        service_shutdown(msg);
        break;
    case AURACAST_SINK_CREATE_SYNC:
        /** Allowed to create new device */
        if ((device = find_or_create_device(msg->id, &msg->addr, msg->sid)) == NULL) {
            BT_LOGE("Failed to create device");
            break;
        }

        auracast_sink_state_machine_handle_event(device->stm, msg);
        break;
    default:
        /** Not allowed to create new device */
        if ((device = find_device_by_msg(msg)) == NULL) {
            BT_LOGE("Device not found");
            break;
        }

        auracast_sink_state_machine_handle_event(device->stm, msg);
        break;
    }

    auracast_sink_msg_destroy(msg);
}

static bt_status_t auracast_sink_init(void)
{
    return BT_STATUS_SUCCESS;
}

static void auracast_sink_cleanup(void)
{
}

static bt_status_t auracast_sink_startup(profile_on_startup_t cb)
{
    auracast_sink_msg_t* msg = auracast_sink_msg_new(AURACAST_SINK_STARTUP, PRIMARY_ADAPTER, NULL,
        BLE_SCAN_SID_NOT_PROVIDED);
    if (!msg) {
        cb(PROFILE_AURACAST_SINK, false);
        return BT_STATUS_FAIL;
    }

    msg->context = cb;
    auracast_sink_send_message(msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t auracast_sink_shutdown(profile_on_shutdown_t cb)
{
    auracast_sink_msg_t* msg = auracast_sink_msg_new(AURACAST_SINK_SHUTDOWN, PRIMARY_ADAPTER, NULL,
        BLE_SCAN_SID_NOT_PROVIDED);
    if (!msg) {
        cb(PROFILE_AURACAST_SINK, false);
        return BT_STATUS_FAIL;
    }

    msg->context = cb;
    auracast_sink_send_message(msg);

    return BT_STATUS_SUCCESS;
}

static int auracast_sink_get_state(void)
{
    return 1;
}

static void* auracast_sink_register_callbacks(void* remote,
    const bt_auracast_sink_callbacks_t* callbacks)
{
    if (!g_auracast_sink_service.callbacks)
        return NULL;

    return bt_remote_callbacks_register(g_auracast_sink_service.callbacks, remote,
        (void*)callbacks);
}

static bool auracast_sink_unregister_callbacks(void** remote, void* cookie)
{
    if (!g_auracast_sink_service.callbacks)
        return false;

    return bt_remote_callbacks_unregister(g_auracast_sink_service.callbacks, remote, cookie);
}

static bt_status_t auracast_sink_dump(void)
{
    auracast_sink_send_message(auracast_sink_msg_new(AURACAST_SINK_DUMP, PRIMARY_ADAPTER, NULL,
        BLE_SCAN_SID_NOT_PROVIDED));

    return BT_STATUS_SUCCESS;
}

static const auracast_sink_interface_t auracast_sink_interface = {
    .register_callbacks = auracast_sink_register_callbacks,
    .unregister_callbacks = auracast_sink_unregister_callbacks,
    .create_sync = auracast_sink_create_sync,
    .terminate_sync = auracast_sink_terminate_sync,
    .dump = auracast_sink_dump,
};

static const void* get_auracast_sink_profile_interface(void)
{
    return &auracast_sink_interface;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
void auracast_sink_send_message(void* msg)
{
    if (!msg)
        return;

    do_in_service_loop(auracast_sink_process_message, msg);
}

void auracast_sink_service_notify_sync_established(const void* context)
{
    const auracast_sink_device_t* device = (const auracast_sink_device_t*)context;

    BT_LOGD("%s", __func__);

    AURACAST_SINK_CALLBACK_FOREACH(g_auracast_sink_service.callbacks, on_sync_established,
        &device->addr, device->sid);
}

void auracast_sink_service_notify_sync_terminated(const void* context)
{
    auracast_sink_device_t* device = (auracast_sink_device_t*)context;

    BT_LOGD("%s", __func__);

    AURACAST_SINK_CALLBACK_FOREACH(g_auracast_sink_service.callbacks, on_sync_terminated,
        &device->addr, device->sid);

    bt_list_remove(g_auracast_sink_service.sink_list, device);

    if (g_auracast_sink_service.terminating)
        auracast_sink_shutdown(g_auracast_sink_service.terminating);
}

static const profile_service_t auracast_sink_service = {
    .auto_start = true,
    .name = PROFILE_AURACAST_SINK_NAME,
    .id = PROFILE_AURACAST_SINK,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = auracast_sink_init,
    .startup = auracast_sink_startup,
    .shutdown = auracast_sink_shutdown,
    .process_msg = NULL,
    .get_state = auracast_sink_get_state,
    .get_profile_interface = get_auracast_sink_profile_interface,
    .cleanup = auracast_sink_cleanup,
    .dump = NULL,
};

void register_auracast_sink_service(void)
{
    register_service(&auracast_sink_service);
}

void auracast_sink_on_established(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid)
{
    auracast_sink_send_message(auracast_sink_msg_new(AURACAST_SINK_SYNC_ESTABLISHED, id, addr, sid));
}

void auracast_sink_on_terminated(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid)
{
    auracast_sink_send_message(auracast_sink_msg_new(AURACAST_SINK_SYNC_TERMINATED, id, addr, sid));
}

void auracast_sink_on_data_received(bt_controller_id_t id, const bt_le_address_t* addr, uint8_t sid,
    uint32_t bis, uint32_t ts, uint16_t seq, uint16_t len, const uint8_t* data)
{
    auracast_sink_event_packet_t* packet;
    auracast_sink_msg_t* msg = auracast_sink_msg_new(AURACAST_SINK_DATA_IN, id, addr, sid);
    if (!msg)
        return;

    packet = malloc(sizeof(auracast_sink_event_packet_t) + len);
    if (!packet) {
        free(msg);
        return;
    }

    packet->bitfield = bis;
    packet->timestamp = ts;
    packet->sequence_number = seq;
    if (len && data) {
        /** TODO: move data to msg->context */
        packet->length = len;
        memcpy(packet->data, data, len);
    } else {
        packet->length = 0;
    }

    msg->payload = packet;

    auracast_sink_send_message(msg);
}

#if HACK_BEFORE_TINYCOMPRESS_DONE
static bool stm_cmp(void* data, void* context)
{
    const auracast_sink_device_t* device = (const auracast_sink_device_t*)data;
    const auracast_sink_state_machine_t* stm = (const auracast_sink_state_machine_t*)context;

    return device->stm == stm;
}

static inline auracast_sink_device_t* find_device_by_stm(const auracast_sink_state_machine_t* stm)
{
    return bt_list_find(g_auracast_sink_service.sink_list, stm_cmp, (void*)stm);
}

void auracast_sink_send_message_delayed(const void* stm, int event, uint16_t size,
    const uint8_t* payload, void* context)
{
    auracast_sink_msg_t* msg;
    auracast_sink_device_t* device;

    device = find_device_by_stm(stm);
    if (!device)
        return;

    msg = auracast_sink_msg_new_ext(event, device->id, &device->addr, device->sid, size);
    if (!msg)
        return;

    if (size && payload) {
        msg->data.size = size;
        memcpy(msg->data.data, payload, size);
    }

    msg->context = context;
    auracast_sink_send_message(msg);
}
#endif
