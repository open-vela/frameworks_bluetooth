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
#define LOG_TAG "lea_client"

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "bt_lea_client.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "index_allocator.h"
#include "lea_audio_sink.h"
#include "lea_audio_source.h"
#include "lea_client_service.h"
#include "lea_client_state_machine.h"
#include "sal_lea_client_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define LOG_TAG "lea_client"

#define CHECK_ENABLED()                    \
    {                                      \
        if (!g_lea_client_service.started) \
            return BT_STATUS_NOT_ENABLED;  \
    }

#define LEAC_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_client_callbacks_t, _cback, ##__VA_ARGS__)

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct
{
    bool started;
    uint8_t max_connections;
    bt_list_t *leac_devices;
    bt_list_t *leac_stream;
    index_allocator_t *index_allocator;
    callbacks_list_t *callbacks;
    pthread_mutex_t device_lock;
    pthread_mutex_t stream_lock;
} lea_client_service_t;

typedef struct {
    bool is_source;
    uint8_t ase_id;
    uint8_t ase_state;
    uint32_t stream_id;
} lea_client_endpoint;

typedef struct
{
    bt_address_t addr;
    uint8_t cs_rank;
    uint8_t cis_id;
    uint8_t ase_number;
    uint8_t pac_number;
    uint32_t group_id;
    uint32_t sink_allocation;
    uint32_t source_allocation;
    uint16_t sink_supported_ctx;
    uint16_t source_supported_ctx;
    uint16_t sink_avaliable_ctx;
    uint16_t source_avaliable_ctx;

    void *cs_set;
    lea_client_state_machine_t *leasm;
    profile_connection_state_t state;
    lea_adpt_context_types_t context;

    lea_client_capability_t pac[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_PAC_MAX_NUMBER];
    lea_client_endpoint ase[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];

} lea_client_device_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static lea_client_state_machine_t *get_state_machine(bt_address_t *addr);
static bt_status_t lea_client_send_message(lea_client_msg_t *msg);

static void on_lea_audio_suspend(uint32_t stream_id);
static void on_lea_audio_resume(uint32_t stream_id);
static void on_lea_meatadata_updated(uint32_t stream_id);
static void on_lea_audio_send(uint32_t stream_id, uint8_t *buffer, uint16_t length);

static void *lea_client_register_callbacks(void *remote, const lea_client_callbacks_t *callbacks);
static bool lea_client_unregister_callbacks(void **remote, void *cookie);
static profile_connection_state_t lea_client_get_connection_state(bt_address_t *addr);
static bt_status_t lea_client_connect_device(bt_address_t *addr);
static bt_status_t lea_client_connect_audio(bt_address_t *addr, uint8_t context);
static bt_status_t lea_client_disconnect_audio(bt_address_t *addr);
static bt_status_t lea_client_disconnect_device(bt_address_t *addr);

/****************************************************************************
 * Private Data
 ****************************************************************************/
static lea_client_service_t g_lea_client_service = {
    .started = false,
    .leac_devices = NULL,
    .callbacks = NULL,
};

static lea_sink_callabcks_t lea_sink_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_meatadata_updated,
    .lea_audio_resume_cb = on_lea_audio_resume,
    .lea_audio_suspend_cb = on_lea_audio_suspend,
};

static lea_source_callabcks_t lea_source_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_meatadata_updated,
    .lea_audio_resume_cb = on_lea_audio_resume,
    .lea_audio_suspend_cb = on_lea_audio_suspend,
    .lea_audio_send_cb = on_lea_audio_send,
};

static const lea_client_interface_t LEAClientInterface = {
    sizeof(LEAClientInterface),
    .register_callbacks = lea_client_register_callbacks,
    .unregister_callbacks = lea_client_unregister_callbacks,
    .connect = lea_client_connect_device,
    .connect_audio = lea_client_connect_audio,
    .disconnect = lea_client_disconnect_device,
    .disconnect_audio = lea_client_disconnect_audio,
    .get_connection_state = lea_client_get_connection_state,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool lea_client_device_cmp(void *device, void *addr)
{
    return bt_addr_compare(&((lea_client_device_t *)device)->addr, addr) == 0;
}

static lea_client_device_t *find_lea_client_device_by_addr(bt_address_t *addr)
{
    return bt_list_find(g_lea_client_service.leac_devices,
                        lea_client_device_cmp, addr);
}

static lea_client_device_t *lea_client_device_new(bt_address_t *addr,
                                                  lea_client_state_machine_t *leasm)
{
    lea_client_device_t *device = calloc(1, sizeof(lea_client_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->leasm = leasm;

    return device;
}

static void lea_client_device_delete(lea_client_device_t *device)
{
    if (!device)
        return;

    lea_client_msg_t *msg = lea_client_msg_new(DISCONNECT_DEVICE, &device->addr);
    if (msg == NULL)
        return;

    lea_client_state_machine_dispatch(device->leasm, msg);
    lea_client_msg_destory(msg);
    lea_client_state_machine_destory(device->leasm);
    free(device);
}

static lea_client_state_machine_t *get_state_machine(bt_address_t *addr)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_state_machine_t *leasm;
    lea_client_device_t *device;

    if (!service->started)
        return NULL;

    device = find_lea_client_device_by_addr(addr);
    if (device)
        return device->leasm;

    leasm = lea_client_state_machine_new(addr, (void *)&g_lea_client_service);
    if (!leasm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    device = lea_client_device_new(addr, leasm);
    if (!device) {
        BT_LOGE("New device alloc failed");
        lea_client_state_machine_destory(leasm);
        return NULL;
    }

    bt_list_add_tail(service->leac_devices, device);

    return leasm;
}

static void lea_client_do_shutdown(void)
{
    lea_client_service_t *service = &g_lea_client_service;

    if (!service->started)
        return;

    pthread_mutex_lock(&service->device_lock);
    service->started = false;
    bt_list_free(service->leac_devices);
    bt_list_free(service->leac_stream);
    service->leac_devices = NULL;
    service->leac_stream = NULL;
    pthread_mutex_unlock(&service->device_lock);

    pthread_mutex_destroy(&service->device_lock);
    index_allocator_delete(service->index_allocator);
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;

    lea_audio_sink_cleanup();
    lea_audio_source_cleanup();
    bt_sal_lea_client_cleanup();
}

static void lea_client_process_message(void *data)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_msg_t *msg = (lea_client_msg_t *)data;

    switch (msg->event) {
    case SHUTDOWN:
        lea_client_do_shutdown();
        break;
    default: {
        pthread_mutex_lock(&service->device_lock);
        lea_client_state_machine_t *leasm = get_state_machine(&msg->data.addr);
        if (leasm)
            lea_client_state_machine_dispatch(leasm, msg);
        pthread_mutex_unlock(&service->device_lock);
        break;
    }
    }

    lea_client_msg_destory(msg);
}

static bt_status_t lea_client_send_message(lea_client_msg_t *msg)
{
    assert(msg);

    do_in_service_loop(lea_client_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_send_event(bt_address_t *addr, lea_client_event_t evt)
{
    lea_client_msg_t *msg = lea_client_msg_new(evt, addr);

    if (!msg)
        return BT_STATUS_NOMEM;

    return lea_client_send_message(msg);
}

static void on_lea_audio_suspend(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGW("%s, failed stream id(0x%08x) invalid", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAM_SUSPEND, &stream->addr);
    if (!msg)
        return;

    BT_LOGD("%s, Stream ID:0x%08x", __func__, stream_id);
    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

static void on_lea_audio_resume(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGW("%s, failed stream id(0x%08x) invalid", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAM_RESUME, &stream->addr);
    if (!msg)
        return;

    BT_LOGD("%s, Stream ID:0x%08x", __func__, stream_id);
    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

static void on_lea_meatadata_updated(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGW("%s, failed stream id(0x%08x) invalid", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_METADATA_UPDATED, &stream->addr);
    if (!msg)
        return;

    BT_LOGD("%s, Stream ID:0x%08x", __func__, stream_id);
    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

static void on_lea_audio_send(uint32_t stream_id, uint8_t *buffer, uint16_t length)
{
    lea_audio_stream_t *stream;
    lea_send_iso_data_t *iso_pkt;
    int size;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        return;
    }

    iso_pkt = bt_sal_lea_alloc_send_buffer(stream->sdu_size, stream->iso_handle);
    iso_pkt->sdu_length = stream->sdu_size;

    size = lea_audio_source_read(stream->stream_id, iso_pkt->sdu, iso_pkt->sdu_length);
    if (size != iso_pkt->sdu_length) {
        BT_LOGE("%s, size:%d frame_size:%d", __func__, size, iso_pkt->sdu_length);
        return;
    }

    bt_sal_lea_send_iso_data(iso_pkt);
}

static bt_status_t lea_client_init(void)
{
    bt_status_t ret;

    ret = lea_audio_sink_init(&lea_sink_callbacks);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    ret = lea_audio_source_init(&lea_source_callbacks);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    return BT_STATUS_SUCCESS;
}

static void lea_client_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static bt_status_t lea_client_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_client_service_t *service = &g_lea_client_service;

    if (service->started)
        return BT_STATUS_SUCCESS;

    service->max_connections = CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_CONNECTIONS;
    service->leac_devices = bt_list_new((bt_list_free_cb_t)lea_client_device_delete);
    service->leac_stream = bt_list_new(NULL);
    service->callbacks = bt_callbacks_list_new(2);
    service->index_allocator = index_allocator_create(CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_ALLOC_NUMBER);
    if (!service->leac_devices || !service->leac_stream || !service->callbacks || !service->index_allocator) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->device_lock, &attr);
    pthread_mutex_init(&service->stream_lock, &attr);

    status = bt_sal_lea_client_init();
    if (status != BT_STATUS_SUCCESS)
        goto fail;

    service->started = true;
    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(service->leac_devices);
    bt_list_free(service->leac_stream);
    index_allocator_delete(service->index_allocator);
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->device_lock);
    pthread_mutex_destroy(&service->stream_lock);
    return status;
}

static bt_status_t lea_client_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);

    return lea_client_send_event(NULL, SHUTDOWN);
}

static void *lea_client_register_callbacks(void *remote, const lea_client_callbacks_t *callbacks)
{
    lea_client_service_t *service = &g_lea_client_service;

    if (!service->started)
        return NULL;

    return bt_remote_callbacks_register(service->callbacks,
                                        remote, (void *)callbacks);
}

static bool lea_client_unregister_callbacks(void **remote, void *cookie)
{
    lea_client_service_t *service = &g_lea_client_service;

    if (!service->started)
        return false;

    return bt_remote_callbacks_unregister(service->callbacks,
                                          remote, cookie);
}

static profile_connection_state_t lea_client_get_connection_state(bt_address_t *addr)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device = find_lea_client_device_by_addr(addr);
    profile_connection_state_t conn_state;

    if (!device)
        return PROFILE_STATE_DISCONNECTED;

    pthread_mutex_lock(&service->device_lock);
    conn_state = device->state;
    pthread_mutex_unlock(&service->device_lock);

    return conn_state;
}

static uint8_t get_current_state_device_count(profile_connection_state_t state)
{
    lea_client_service_t *service = &g_lea_client_service;
    bt_list_t *list = service->leac_devices;
    bt_list_node_t *node;
    uint8_t cnt = 0;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        lea_client_device_t *device = bt_list_node(node);
        if (lea_client_get_connection_state(&device->addr) >= state)
            cnt++;
    }

    return cnt;
}

static bt_status_t lea_client_connect_device(bt_address_t *addr)
{
    lea_client_service_t *service = &g_lea_client_service;
    CHECK_ENABLED();

    if (get_current_state_device_count(PROFILE_STATE_CONNECTING) > service->max_connections) {
        return BT_STATUS_NO_RESOURCES;
    }

    return lea_client_send_event(addr, CONNECT_DEVICE);
}

static bt_status_t lea_client_get_group_id(bt_address_t *addr, uint32_t *group_id)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;
    bt_status_t ret;
    int salt;

    CHECK_ENABLED();

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    pthread_mutex_lock(&service->device_lock);
    *group_id = device->group_id;
    pthread_mutex_unlock(&service->device_lock);
    if (*group_id > 0) {
        return BT_STATUS_SUCCESS;
    }

    salt = index_alloc(service->index_allocator);
    if (salt < 0) {
        BT_LOGE("%s, index_alloc(%d) failed", __func__, salt);
        return BT_STATUS_ERROR_BUT_UNKNOWN;
    }

    ret = bt_sal_lea_ucc_group_create(group_id, (uint8_t)salt, NULL, NULL);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, group_create failed(%d)", __func__, ret);
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_get_ases_stream_id(bt_address_t *addr, uint8_t *num, uint32_t *stream_ids)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;
    int index;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    pthread_mutex_lock(&service->device_lock);
    for (index = 0; index < device->ase_number; index++) {
        stream_ids[index] = device->ase[index].stream_id;
    }
    *num = index;
    pthread_mutex_unlock(&service->device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_connect_audio(bt_address_t *addr, uint8_t context)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;
    uint32_t group_id;
    bt_status_t ret;
    profile_connection_state_t state;
    lea_client_msg_t *msg;

    CHECK_ENABLED();

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&service->device_lock);
    state = device->state;
    pthread_mutex_unlock(&service->device_lock);
    if (state != PROFILE_STATE_CONNECTED) {
        BT_LOGE("%s, device no connected", __func__);
        return BT_STATUS_NO_RESOURCES;
    }

    ret = lea_client_get_group_id(&device->addr, &group_id);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    pthread_mutex_lock(&service->device_lock);
    device->group_id = group_id;
    device->context = context;
    pthread_mutex_unlock(&service->device_lock);

    msg = lea_client_msg_new(CONNECT_AUDIO, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = context;
    return lea_client_send_message(msg);
}

static bt_status_t lea_client_disconnect_device(bt_address_t *addr)
{
    CHECK_ENABLED();
    profile_connection_state_t state = lea_client_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    return lea_client_send_event(addr, DISCONNECT_DEVICE);
}

static bt_status_t lea_client_disconnect_audio(bt_address_t *addr)
{
    lea_client_msg_t *msg = lea_client_msg_new(DISCONNECT_AUDIO, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    return lea_client_send_message(msg);
}

static bt_status_t lea_client_group_delete(uint32_t gid)
{
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_group_remove(bt_address_t *addr)
{
    lea_client_device_t *device;
    uint8_t number;
    bt_status_t ret;
    uint32_t stream_ids[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    ret = lea_client_get_ases_stream_id(&device->addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, device no exist", __func__);
        return ret;
    }

    if (number > CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
                CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_remove_stream(device->group_id, number, stream_ids);
}

static const void *get_leac_profile_interface(void)
{
    return &LEAClientInterface;
}

static int lea_client_dump(void)
{
    printf("impl hfp hf dump");
    return 0;
}

static bool lea_client_stream_cmp(void *audio_stream, void *stream_id)
{
    return ((lea_audio_stream_t *)audio_stream)->stream_id == *((uint32_t *)stream_id);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lea_audio_stream_t *lea_client_add_stream(
    uint32_t stream_id, bt_address_t *remote_addr)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_audio_stream_t *audio_stream;

    audio_stream = malloc(sizeof(lea_audio_stream_t));
    if (!audio_stream) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }

    audio_stream->stream_id = stream_id;
    audio_stream->is_source = bt_sal_lea_is_source_stream(stream_id);
    memcpy(&audio_stream->addr, remote_addr, sizeof(bt_address_t));

    pthread_mutex_lock(&service->stream_lock);
    bt_list_add_tail(service->leac_stream, audio_stream);
    pthread_mutex_unlock(&service->stream_lock);

    return audio_stream;
}

lea_audio_stream_t *lea_client_find_stream(uint32_t stream_id)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_audio_stream_t *stream;

    pthread_mutex_lock(&service->stream_lock);
    stream = bt_list_find(service->leac_stream, lea_client_stream_cmp, &stream_id);
    pthread_mutex_unlock(&service->stream_lock);

    return stream;
}

lea_audio_stream_t *lea_client_find_update_stream(lea_audio_stream_t *stream)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_audio_stream_t *local_stream;

    pthread_mutex_lock(&service->stream_lock);
    local_stream = bt_list_find(service->leac_stream, lea_client_stream_cmp, &stream->stream_id);
    if (local_stream) {
        memcpy(local_stream, stream, sizeof(lea_audio_stream_t));
    }
    pthread_mutex_unlock(&service->stream_lock);

    return local_stream;
}

void lea_client_remove_stream(uint32_t stream_id)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_audio_stream_t *audio_stream;

    pthread_mutex_lock(&service->stream_lock);
    audio_stream = bt_list_find(service->leac_stream, lea_client_stream_cmp, &stream_id);
    bt_list_remove(service->leac_stream, audio_stream);
    pthread_mutex_unlock(&service->stream_lock);
}

void lea_client_remove_streams()
{
    lea_client_service_t *service = &g_lea_client_service;

    pthread_mutex_lock(&service->stream_lock);
    bt_list_clear(service->leac_stream);
    pthread_mutex_unlock(&service->stream_lock);
}

void lea_client_notify_stack_state_changed(lea_client_stack_state_t
                                               enabled)
{
    lea_client_service_t *service = &g_lea_client_service;

    LEAC_CALLBACK_FOREACH(service->callbacks, client_stack_state_cb, enabled);
}

void lea_client_notify_connection_state_changed(bt_address_t *addr,
                                                profile_connection_state_t state)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    device->state = state;
    pthread_mutex_unlock(&service->device_lock);

    LEAC_CALLBACK_FOREACH(service->callbacks, client_connection_state_cb, state, addr);
}

static bt_status_t lea_client_ucc_get_prefer_stream(bt_address_t *addr, lea_client_endpoint *endpoint, lea_audio_stream_t *stream)
{
    lea_client_device_t *device;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    // todo prefer from current context and remote capabilty
    memcpy(&stream->addr, addr, sizeof(bt_address_t));
    stream->stream_id = endpoint->stream_id;
    stream->target_latency = ADPT_LEA_ASE_TARGET_BALANCED;
    stream->target_phy = ADPT_LEA_ASE_TARGET_PHY_2M;

    stream->codec_cfg.codec_id.format = 0x06; // LC3
    stream->codec_cfg.frequency = 6; // 32k
    stream->codec_cfg.duration = 1; // 10ms
    stream->codec_cfg.octets = 80;
    stream->codec_cfg.blocks = 1;
    stream->codec_cfg.allocation = 0x01;

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_alloc_cis_id(uint8_t *cis_id)
{
    lea_client_service_t *service = &g_lea_client_service;
    int salt;

    salt = index_alloc(service->index_allocator);
    if (salt < 0) {
        BT_LOGE("%s, index_alloc(%d) failed", __func__, salt);
        return BT_STATUS_ERROR_BUT_UNKNOWN;
    }

    *cis_id = (uint8_t)salt;
    return BT_STATUS_SUCCESS;
}

static void lea_client_free_cis_id(uint8_t cis_id)
{
    lea_client_service_t *service = &g_lea_client_service;

    index_free(service->index_allocator, cis_id);
}

static bt_status_t lea_client_get_stream_id(bt_address_t *addr, lea_client_endpoint *endpoint, uint8_t cis_id,
                                            uint32_t *stream_id)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;

    CHECK_ENABLED();

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    pthread_mutex_lock(&service->device_lock);
    *stream_id = endpoint->stream_id;
    pthread_mutex_unlock(&service->device_lock);
    if (*stream_id > 0) {
        return BT_STATUS_SUCCESS;
    }

    bt_sal_lea_alloc_stream_id(device->group_id, cis_id, endpoint->ase_id, endpoint->is_source, stream_id);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_client_ucc_add_streams(bt_address_t *addr)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;
    lea_audio_stream_t stream;
    int index;
    uint8_t cis_id;
    bt_status_t ret;
    uint32_t stream_id;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    pthread_mutex_lock(&service->device_lock);
    if (!device->cis_id) {
        ret = lea_client_alloc_cis_id(&cis_id);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, alloc_cis_id failed", __func__);
            return ret;
        }
        device->cis_id = cis_id;
    }

    for (index = 0; index < device->ase_number; index++) {
        ret = lea_client_get_stream_id(&device->addr, &device->ase[index], device->cis_id, &stream_id);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, get_stream_id failed", __func__);
            return ret;
        }
        device->ase[index].stream_id = stream_id;
        if (lea_client_ucc_get_prefer_stream(&device->addr, &device->ase[index], &stream) == BT_STATUS_SUCCESS) {
            bt_sal_lea_ucc_group_add_stream(device->group_id, &stream);
        }
    }
    pthread_mutex_unlock(&service->device_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t lea_client_ucc_remove_streams(bt_address_t *addr)
{
    return lea_client_group_remove(addr);
}

bt_status_t lea_client_ucc_config_codec(bt_address_t *addr)
{
    lea_client_device_t *device;
    uint8_t number;
    uint32_t stream_ids[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];
    bt_status_t ret;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    ret = lea_client_get_ases_stream_id(&device->addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, device no exist", __func__);
        return ret;
    }

    if (number > CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
                CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_request_codec(device->group_id, number, stream_ids);
}

bt_status_t lea_client_ucc_config_qos(bt_address_t *addr)
{
    lea_client_device_t *device;
    uint8_t number;
    uint32_t stream_ids[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];
    bt_status_t ret;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    ret = lea_client_get_ases_stream_id(&device->addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, device no exist", __func__);
        return ret;
    }

    if (number > CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
                CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_request_qos(device->group_id, number,
                                            stream_ids);
}

bt_status_t lea_client_ucc_enable(bt_address_t *addr)
{
    lea_client_device_t *device;
    int index;
    uint8_t number;
    bt_status_t ret;
    lea_metadata_t metadata[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_METADATA_MAX_NUMBER];
    uint32_t stream_ids[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    ret = lea_client_get_ases_stream_id(&device->addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, device no exist", __func__);
        return ret;
    }

    if (number > CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
                CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER);
        return BT_STATUS_NOMEM;
    }

    for (index = 0; index < device->ase_number; index++) {
        // todo prefer from current ctx and remote/local supported ctx
        metadata[index].streaming_contexts = ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL;
        metadata[index].type = ADPT_LEA_METADATA_PREFERRED_AUDIO_CONTEXTS;
    }

    // todo prefer streams according to context
    return bt_sal_lea_ucc_group_request_enable(device->group_id, device->context, stream_ids, metadata);
}

bt_status_t lea_client_ucc_disable(bt_address_t *addr)
{
    lea_client_device_t *device;
    uint8_t number;
    uint32_t stream_ids[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];
    bt_status_t ret;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    ret = lea_client_get_ases_stream_id(&device->addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, device no exist", __func__);
        return ret;
    }

    if (number > CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
                CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_request_disable(device->group_id, number,
                                                stream_ids);
}

void lea_client_on_stack_state_changed(lea_client_stack_state_t enabled)
{
    lea_client_msg_t *msg = lea_client_msg_new(STACK_EVENT_STACK_STATE,
                                               NULL);
    if (!msg)
        return;

    msg->data.valueint1 = enabled;
    lea_client_send_message(msg);
}

void lea_client_on_connection_state_changed(bt_address_t *addr,
                                            profile_connection_state_t state)
{
    lea_client_msg_t *msg = lea_client_msg_new(STACK_EVENT_CONNECTION_STATE,
                                               addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    lea_client_send_message(msg);
}

void lea_client_on_storage_changed(void *value, uint32_t size)
{
    lea_client_msg_t *msg = lea_client_msg_new_ext(STACK_EVENT_STORAGE, NULL,
                                                   size);
    if (!msg)
        return;

    memcpy(msg->data.dataarry, value, size);
    lea_client_send_message(msg);
}

void lea_client_on_pac_event(bt_address_t *addr, lea_client_capability_t *cap)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    memcpy(&device->pac[device->pac_number], cap, sizeof(lea_client_capability_t));
    device->pac_number++;
    pthread_mutex_unlock(&service->device_lock);
}

void lea_client_on_ascs_event(bt_address_t *addr, uint8_t ase_state, bool is_source, uint8_t ase_id)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;
    int index;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    if (ase_state == ADPT_LEA_ASE_STATE_IDLE && device->ase_number < CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER) {
        device->ase[device->ase_number].is_source = is_source;
        device->ase[device->ase_number].ase_id = ase_id;
        device->ase_number++;
        pthread_mutex_unlock(&service->device_lock);
        return;
    }

    for (index = 0; index < device->ase_number; index++) {
        if (device->ase[index].ase_id == ase_id) {
            device->ase[index].ase_state = ase_state;
            break;
        }
    }

    pthread_mutex_unlock(&service->device_lock);
}

void lea_client_on_ascs_completed(bt_address_t *addr, uint32_t stream_id, uint8_t operation, uint8_t status)
{
    lea_client_device_t *device;
    lea_client_event_t event;
    lea_client_msg_t *msg;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    switch (operation) {
    case LEA_ASE_OP_CONFIG_CODEC: {
        event = STACK_EVENT_ASE_CODEC_CONFIG;
        break;
    }
    case LEA_ASE_OP_CONFIG_QOS: {
        event = STACK_EVENT_ASE_QOS_CONFIG;
        break;
    }
    case LEA_ASE_OP_ENABLE: {
        event = STACK_EVENT_ASE_ENABLING;
        break;
    }
    case LEA_ASE_OP_DISABLE: {
        event = STACK_EVENT_ASE_DISABLING;
        break;
    }
    case LEA_ASE_OP_RELEASE: {
        event = STACK_EVENT_ASE_RELEASING;
        break;
    }
    case LEA_ASE_OP_UPDATE_METADATA: {
        event = STACK_EVENT_METADATA_UPDATED;
        break;
    }
    default: {
        BT_LOGE("%s, unexpect op:%d", __func__, operation);
        return;
    };
    }

    msg = lea_client_msg_new(event, &device->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    msg->data.valueint2 = status;
    lea_client_send_message(msg);
}

void lea_client_on_audio_localtion_event(bt_address_t *addr, bool is_source, uint32_t allcation)
{

    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    if (is_source) {
        device->source_allocation = allcation;
    } else {
        device->sink_allocation = allcation;
    }
    pthread_mutex_unlock(&service->device_lock);
}

void lea_client_on_available_audio_contexts_event(bt_address_t *addr, uint32_t sink_ctxs, uint32_t source_ctxs)
{

    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    device->source_avaliable_ctx = source_ctxs;
    device->sink_avaliable_ctx = sink_ctxs;
    pthread_mutex_unlock(&service->device_lock);
}

void lea_client_on_supported_audio_contexts_event(bt_address_t *addr, uint32_t sink_ctxs, uint32_t source_ctxs)
{
    lea_client_service_t *service = &g_lea_client_service;
    lea_client_device_t *device;

    device = find_lea_client_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, devicenot exist", __func__);
        return;
    }

    pthread_mutex_lock(&service->device_lock);
    device->source_supported_ctx = source_ctxs;
    device->sink_supported_ctx = sink_ctxs;
    pthread_mutex_unlock(&service->device_lock);
}

void lea_client_on_stream_added(bt_address_t *addr, uint32_t stream_id)
{
    lea_client_msg_t *msg = lea_client_msg_new(STACK_EVENT_STREAM_ADDED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_removed(bt_address_t *addr, uint32_t stream_id)
{
    lea_client_msg_t *msg = lea_client_msg_new(STACK_EVENT_STREAM_REMOVED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_started(lea_audio_stream_t *audio)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(audio->stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, audio->stream_id);
        return;
    }

    msg = lea_client_msg_new_ext(STACK_EVENT_STREAM_STARTED,
                                 &stream->addr, sizeof(lea_audio_stream_t));
    if (!msg)
        return;

    memcpy(msg->data.dataarry, audio, sizeof(lea_audio_stream_t));
    lea_client_send_message(msg);
}

void lea_client_on_stream_stopped(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAM_STOPPED, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_suspend(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAM_SUSPEND, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_resume(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAM_RESUME, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_metedata_updated(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_METADATA_UPDATED, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_recv(uint32_t stream_id, uint32_t time_stamp,
                               uint16_t seq_number, uint8_t *sdu, uint16_t size)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;
    lea_recv_iso_data_t *packet;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAN_RECV, &stream->addr);
    if (!msg)
        return;

    packet = lea_audio_sink_packet_alloc(time_stamp, seq_number, sdu, size);
    if (!packet)
        return;

    msg->data.valueint1 = stream_id;
    msg->data.datapointer = packet;
    lea_client_send_message(msg);
}

void lea_client_on_stream_send(uint32_t stream_id)
{
    lea_audio_stream_t *stream;
    lea_client_msg_t *msg;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    msg = lea_client_msg_new(STACK_EVENT_STREAN_SENT, &stream->addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

static const profile_service_t lea_client_service = {
    .auto_start = true,
    .name = "lea_client",
    .id = PROFILE_LEAUDIO_CLIENT,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = lea_client_init,
    .startup = lea_client_startup,
    .shutdown = lea_client_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_leac_profile_interface,
    .cleanup = lea_client_cleanup,
    .dump = lea_client_dump,
};

void register_lea_client_service(void)
{
    register_service(&lea_client_service);
}