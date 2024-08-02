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
#include <debug.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "bt_lea_client.h"
#include "bt_profile.h"
#include "bt_vendor.h"
#include "callbacks_list.h"
#include "index_allocator.h"
#include "lea_audio_sink.h"
#include "lea_audio_source.h"
#include "lea_client_service.h"
#include "lea_client_state_machine.h"
#include "sal_lea_client_interface.h"
#include "sal_lea_common.h"
#include "sal_lea_csip_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CHECK_ENABLED()                    \
    {                                      \
        if (!g_lea_client_service.started) \
            return BT_STATUS_NOT_ENABLED;  \
    }

#define LEAC_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_client_callbacks_t, _cback, ##__VA_ARGS__)

#define LEA_CLIENT_SRIK_SIZE 16
#define LEA_CLIENT_GROUP_ID_DEFAULT 0

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef enum {
    LEA_ASCS_OP_IDLE,
    LEA_ASCS_OP_CODEC,
    LEA_ASCS_OP_QOS,
    LEA_ASCS_OP_ENABLING,
    LEA_ASCS_OP_STREAMING,
    LEA_ASCS_OP_DISABLING,
    LEA_ASCS_OP_RELEASING,
} lea_client_ascs_op_t;

typedef struct {
    bool is_source;
    bool active;
    uint8_t ase_id;
    uint8_t ase_state;
    uint32_t stream_id;
    lea_client_ascs_op_t op;
} lea_client_endpoint_t;

typedef struct
{
    bt_address_t addr;

    uint8_t cs_rank;
    uint8_t cis_id;
    uint8_t ase_number;
    uint8_t pac_number;
    uint16_t sink_supported_ctx;
    uint16_t source_supported_ctx;
    uint16_t sink_avaliable_ctx;
    uint16_t source_avaliable_ctx;
    uint32_t sink_allocation;
    uint32_t source_allocation;

    lea_client_state_machine_t* leasm;
    profile_connection_state_t state;
    pthread_mutex_t device_lock;

    lea_client_capability_t pac[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_PAC_MAX_NUMBER];
    lea_client_endpoint_t ase[CONFIG_BLUETOOTH_LEAUDIO_CLIENT_ASE_MAX_NUMBER];
} lea_client_device_t;

typedef struct {
    uint8_t cs_size;
    lea_client_ascs_op_t op;
    uint8_t sirk[LEA_CLIENT_SRIK_SIZE];

    uint32_t group_id;
    bt_list_t* devices;
    uint8_t context;
} lea_client_group_t;

typedef struct
{
    bool started;
    bool offloading;
    uint8_t max_connections;
    bt_list_t* leac_streams;
    bt_list_t* leac_groups;
    index_allocator_t* index_allocator;
    callbacks_list_t* callbacks;
    pthread_mutex_t group_lock;
    pthread_mutex_t stream_lock;
} lea_client_service_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/
static lea_client_state_machine_t* get_state_machine(bt_address_t* addr);
static lea_client_group_t* find_group_by_id(uint32_t group_id);
static void client_startup(void* data);

static void on_lea_sink_audio_suspend(void);
static void on_lea_sink_audio_resume(void);
static void on_lea_sink_meatadata_updated(void);

static void on_lea_source_audio_suspend(void);
static void on_lea_source_audio_resume(void);
static void on_lea_source_meatadata_updated(void);
static void on_lea_source_audio_send(uint8_t* buffer, uint16_t length);

static void* lea_client_register_callbacks(void* remote, const lea_client_callbacks_t* callbacks);
static bool lea_client_unregister_callbacks(void** remote, void* cookie);
static profile_connection_state_t lea_client_get_connection_state(bt_address_t* addr);
static bt_status_t lea_client_connect_device(bt_address_t* addr);
static bt_status_t lea_client_connect_audio(bt_address_t* addr, uint8_t context);
static bt_status_t lea_client_disconnect_audio(bt_address_t* addr);
static bt_status_t lea_client_disconnect_device(bt_address_t* addr);
static bt_status_t lea_client_get_group_id(bt_address_t* addr, uint32_t* group_id);
static bt_status_t lea_client_discovery_member_start(uint32_t group_id);
static bt_status_t lea_client_discovery_member_stop(uint32_t group_id);
static bt_status_t lea_client_group_add_member(uint32_t group_id, bt_address_t* addr);
static bt_status_t lea_client_group_remove_member(uint32_t group_id, bt_address_t* addr);
static bt_status_t lea_client_group_connect_audio(uint32_t group_id, uint8_t context);
static bt_status_t lea_client_group_disconnect_audio(uint32_t group_id);
static bt_status_t lea_client_group_lock(uint32_t group_id);
static bt_status_t lea_client_group_unlock(uint32_t group_id);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
bt_status_t lea_client_send_message(lea_client_msg_t* msg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const lea_lc3_config_t g_lea_lc3_configs[] = {
    /* Index starts from 1: Odd is 7.5 ms; Even is 10 ms. */
    { 0,
        0,
        0 },

    /* ADPT_LC3SET_8_1_1 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_8000,
        ADPT_LEA_FRAME_DURATION_7_5,
        26 },

    /* ADPT_LEA_LC3_SET_8_2_2 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_8000,
        ADPT_LEA_FRAME_DURATION_10,
        30 },

    /* ADPT_LEA_LC3_SET_16_1_3 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_16000,
        ADPT_LEA_FRAME_DURATION_7_5,
        30 },

    /* ADPT_LEA_LC3_SET_16_2_4 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_16000,
        ADPT_LEA_FRAME_DURATION_10,
        40 },

    /* ADPT_LEA_LC3_SET_24_1_5 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_24000,
        ADPT_LEA_FRAME_DURATION_7_5,
        45 },

    /* ADPT_LEA_LC3_SET_24_2_6 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_24000,
        ADPT_LEA_FRAME_DURATION_10,
        60 },

    /* ADPT_LEA_LC3_SET_32_1_7 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_32000,
        ADPT_LEA_FRAME_DURATION_7_5,
        60 },

    /* ADPT_LEA_LC3_SET_32_2_8 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_32000,
        ADPT_LEA_FRAME_DURATION_10,
        80 },

    /* ADPT_LEA_LC3_SET_441_1_9 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_44100,
        ADPT_LEA_FRAME_DURATION_7_5,
        97 },

    /* ADPT_LEA_LC3_SET_441_2_10 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_44100,
        ADPT_LEA_FRAME_DURATION_10,
        130 },

    /* ADPT_LEA_LC3_SET_48_1_11 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_48000,
        ADPT_LEA_FRAME_DURATION_7_5,
        75 },

    /* ADPT_LEA_LC3_SET_48_2_12 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_48000,
        ADPT_LEA_FRAME_DURATION_10,
        100 },

    /* ADPT_LEA_LC3_SET_48_3_13 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_48000,
        ADPT_LEA_FRAME_DURATION_7_5,
        90 },

    /* ADPT_LEA_LC3_SET_48_4_14 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_48000,
        ADPT_LEA_FRAME_DURATION_10,
        120 },

    /* ADPT_LEA_LC3_SET_48_5_15 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_48000,
        ADPT_LEA_FRAME_DURATION_7_5,
        117 },

    /* ADPT_LEA_LC3_SET_48_6_16 */
    {
        ADPT_LEA_SAMPLE_FREQUENCY_48000,
        ADPT_LEA_FRAME_DURATION_10,
        155 },
};

static const uint8_t g_voice_context_config[] = { ADPT_LEA_LC3_SET_16_2_4, ADPT_LEA_LC3_SET_8_2_2, ADPT_LEA_LC3_SET_24_2_6 };
static const uint8_t g_media_context_config[] = { ADPT_LEA_LC3_SET_32_2_8, ADPT_LEA_LC3_SET_24_2_6, ADPT_LEA_LC3_SET_16_2_4 };
static const uint8_t g_live_context_config[] = { ADPT_LEA_LC3_SET_16_2_4, ADPT_LEA_LC3_SET_24_2_6, ADPT_LEA_LC3_SET_32_2_8 };

static const lea_lc3_prefer_config g_lc3_local_prefer_configs[] = {
    { ADPT_LEA_CONTEXT_TYPE_CONVERSATIONAL | ADPT_LEA_CONTEXT_TYPE_INSTRUCTIONAL | ADPT_LEA_CONTEXT_TYPE_VOICE_ASSISTANTS | ADPT_LEA_CONTEXT_TYPE_SOUND_EFFECTS | ADPT_LEA_CONTEXT_TYPE_NOTIFICATIONS | ADPT_LEA_CONTEXT_TYPE_RINGTONE | ADPT_LEA_CONTEXT_TYPE_ALERTS | ADPT_LEA_CONTEXT_TYPE_EMERGENCY_ALARM,
        sizeof(g_voice_context_config), (uint8_t*)g_voice_context_config },

    { ADPT_LEA_CONTEXT_TYPE_MEDIA,
        sizeof(g_media_context_config), (uint8_t*)g_media_context_config },

    { ADPT_LEA_CONTEXT_TYPE_UNSPECIFIED | ADPT_LEA_CONTEXT_TYPE_GAME | ADPT_LEA_CONTEXT_TYPE_LIVE,
        sizeof(g_live_context_config), (uint8_t*)g_live_context_config },
};

static lea_client_service_t g_lea_client_service = {
    .started = false,
    .leac_streams = NULL,
    .leac_groups = NULL,
    .callbacks = NULL,
};

static lea_sink_callabcks_t lea_sink_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_sink_meatadata_updated,
    .lea_audio_resume_cb = on_lea_sink_audio_resume,
    .lea_audio_suspend_cb = on_lea_sink_audio_suspend,
};

static lea_source_callabcks_t lea_source_callbacks = {
    .lea_audio_meatadata_updated_cb = on_lea_source_meatadata_updated,
    .lea_audio_resume_cb = on_lea_source_audio_resume,
    .lea_audio_suspend_cb = on_lea_source_audio_suspend,
    .lea_audio_send_cb = on_lea_source_audio_send,
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
    .get_group_id = lea_client_get_group_id,
    .discovery_member_start = lea_client_discovery_member_start,
    .discovery_member_stop = lea_client_discovery_member_stop,
    .group_add_member = lea_client_group_add_member,
    .group_remove_member = lea_client_group_remove_member,
    .group_connect_audio = lea_client_group_connect_audio,
    .group_disconnect_audio = lea_client_group_disconnect_audio,
    .group_lock = lea_client_group_lock,
    .group_unlock = lea_client_group_unlock,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool device_addr_cmp_cb(void* device, void* addr)
{
    return bt_addr_compare(&((lea_client_device_t*)device)->addr, addr) == 0;
}

static lea_client_device_t* find_device_by_group_addr(lea_client_group_t* group, bt_address_t* addr)
{
    return bt_list_find(group->devices, device_addr_cmp_cb, addr);
}

static lea_client_device_t* find_device_by_groupid_addr(uint32_t group_id, bt_address_t* addr)
{
    lea_client_group_t* group;

    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group(%u) not found", __func__, group_id);
        return NULL;
    }

    return find_device_by_group_addr(group, addr);
}

static lea_client_group_t* find_group_by_addr(bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_t* list = service->leac_groups;
    lea_client_group_t* group;
    lea_client_device_t* device;
    bt_list_node_t* node;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        group = bt_list_node(node);
        device = find_device_by_group_addr(group, addr);
        if (device) {
            return group;
        }
    }
    return NULL;
}

static lea_client_device_t* find_device_by_addr(bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_t* list = service->leac_groups;
    lea_client_group_t* group;
    lea_client_device_t* device;
    bt_list_node_t* node;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        group = bt_list_node(node);
        device = find_device_by_group_addr(group, addr);
        if (device) {
            return device;
        }
    }
    return NULL;
}

static lea_client_endpoint_t* find_ase_by_device_streamid(lea_client_device_t* device, uint32_t stream_id)
{
    lea_client_endpoint_t* ase;
    int index;

    for (index = 0; index < device->ase_number; index++) {
        ase = &device->ase[index];
        if (ase->stream_id == stream_id) {
            return ase;
        }
    }

    return NULL;
}

static lea_client_device_t* lea_client_device_new(bt_address_t* addr,
    lea_client_state_machine_t* leasm)
{
    pthread_mutexattr_t attr;
    lea_client_device_t* device = calloc(1, sizeof(lea_client_device_t));
    if (!device)
        return NULL;

    memcpy(&device->addr, addr, sizeof(bt_address_t));
    device->leasm = leasm;

    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&device->device_lock, &attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    return device;
}

static void lea_client_device_delete(lea_client_device_t* device)
{
    if (!device)
        return;

    lea_client_msg_t* msg = lea_client_msg_new(DISCONNECT_DEVICE, &device->addr);
    if (msg == NULL)
        return;

    lea_client_state_machine_dispatch(device->leasm, msg);
    lea_client_msg_destory(msg);
    lea_client_state_machine_destory(device->leasm);
    pthread_mutex_destroy(&device->device_lock);
    free(device);
}

static bool group_sirk_cmp_cb(void* data, void* sirk)
{
    lea_client_group_t* group = data;

    return (sirk != NULL) && (memcmp(group->sirk, sirk, LEA_CLIENT_SRIK_SIZE) == 0);
}

static lea_client_group_t* find_group_by_sirk(uint8_t* sirk)
{
    lea_client_service_t* service = &g_lea_client_service;

    return bt_list_find(service->leac_groups, group_sirk_cmp_cb, sirk);
}

static bool find_group_id_cb(void* data, void* group_id)
{
    lea_client_group_t* group = (lea_client_group_t*)data;

    return group->group_id == *((int*)group_id);
}

static lea_client_group_t* find_group_by_id(uint32_t group_id)
{
    lea_client_service_t* service = &g_lea_client_service;

    return bt_list_find(service->leac_groups, find_group_id_cb, &group_id);
}

static void group_delete_cb(void* data)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group = (lea_client_group_t*)data;

    bt_list_clear(group->devices);
    index_free(service->index_allocator, group->group_id);
    bt_sal_lea_ucc_group_delete(group->group_id);
}

static void lea_client_group_delete(uint8_t* sirk)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;

    group = find_group_by_sirk(sirk);
    if (!group) {
        BT_LOGE("%s, group not found", __func__);
        return;
    }

    pthread_mutex_lock(&service->group_lock);
    bt_list_remove(service->leac_groups, group);
    pthread_mutex_unlock(&service->group_lock);
}

static lea_client_group_t* lea_client_group_new(uint8_t* sirk)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;
    int salt;
    bt_status_t ret;

    group = calloc(1, sizeof(lea_client_group_t));
    if (!group) {
        BT_LOGE("%s, malloc fail", __func__);
        return NULL;
    }

    salt = index_alloc(service->index_allocator);
    if (salt < 0) {
        BT_LOGE("%s, index_alloc(%d) failed", __func__, salt);
        return NULL;
    }

    group->devices = bt_list_new((bt_list_free_cb_t)lea_client_device_delete);
    pthread_mutex_lock(&service->group_lock);
    bt_list_add_tail(service->leac_groups, group);
    pthread_mutex_unlock(&service->group_lock);

    if (!sirk) {
        group->group_id = LEA_CLIENT_GROUP_ID_DEFAULT;
        return group;
    }

    memcpy(group->sirk, sirk, LEA_CLIENT_SRIK_SIZE);
    ret = bt_sal_lea_ucc_group_create(&group->group_id, (uint8_t)salt, NULL, NULL);
    if (ret != BT_STATUS_SUCCESS) {
        lea_client_group_delete(sirk);
        BT_LOGE("%s, group_create failed(%d)", __func__, ret);
        return NULL;
    }

    return group;
}

static lea_client_group_t* group_add_member(uint32_t group_id, lea_client_device_t* device)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;

    pthread_mutex_lock(&service->group_lock);
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group not found", __func__);
        pthread_mutex_unlock(&service->group_lock);
        return NULL;
    }

    bt_list_add_tail(group->devices, device);
    pthread_mutex_unlock(&service->group_lock);

    return group;
}

static bt_status_t group_remove_member(uint32_t group_id, bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;
    lea_client_device_t* device;

    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    device = find_device_by_group_addr(group, addr);
    if (!device) {
        BT_LOGE("%s, device not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&service->group_lock);
    bt_list_remove(group->devices, device);
    pthread_mutex_unlock(&service->group_lock);

    return BT_STATUS_SUCCESS;
}

static void group_move_member(lea_client_group_t* src, lea_client_group_t* des, bt_address_t* addr)
{
    lea_client_device_t* device;

    if (!src || !des) {
        BT_LOGE("%s, group null", __func__);
        return;
    }

    device = find_device_by_group_addr(src, addr);
    if (!device) {
        BT_LOGE("%s, device not found", __func__);
        return;
    }

    bt_list_move(src->devices, des->devices, device, false);
}

static bool check_group_completed_by_op(uint32_t group_id, lea_client_ascs_op_t op)
{
    bt_list_t* list;
    lea_client_device_t* device;
    bt_list_node_t* node;
    lea_client_group_t* group;
    int index;

    group = find_group_by_id(group_id);
    if (!group) {
        return false;
    }

    if (group->op == op) {
        return true;
    }

    list = group->devices;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        device = bt_list_node(node);
        for (index = 0; index < device->ase_number; index++) {
            if (device->ase[index].active && (device->ase[index].op != op)) {
                return false;
            }
        }
    }

    group->op = op;
    return true;
}

static bool check_group_completed_by_state(uint32_t group_id, lea_adpt_ase_state_t state)
{
    bt_list_t* list;
    lea_client_device_t* device;
    bt_list_node_t* node;
    lea_client_group_t* group;
    int index;

    group = find_group_by_id(group_id);
    if (!group) {
        return false;
    }

    list = group->devices;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        device = bt_list_node(node);
        for (index = 0; index < device->ase_number; index++) {
            BT_LOGD("%s active:%d, ase state:%d, state:%d", __func__, device->ase[index].active, device->ase[index].ase_state, state);
            if (device->ase[index].active && (device->ase[index].ase_state != state)) {
                return false;
            }
        }
    }

    return true;
}

static lea_client_state_machine_t* get_state_machine(bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_state_machine_t* leasm;
    lea_client_device_t* device;

    if (!service->started)
        return NULL;

    device = find_device_by_addr(addr);
    if (device)
        return device->leasm;

    leasm = lea_client_state_machine_new(addr, (void*)&g_lea_client_service);
    if (!leasm) {
        BT_LOGE("Create state machine failed");
        return NULL;
    }

    lea_client_state_machine_set_offloading(leasm, service->offloading);
    device = lea_client_device_new(addr, leasm);
    if (!device) {
        BT_LOGE("New device alloc failed");
        lea_client_state_machine_destory(leasm);
        return NULL;
    }

    group_add_member(LEA_CLIENT_GROUP_ID_DEFAULT, device);
    return leasm;
}

static void lea_client_do_shutdown(void)
{
    lea_client_service_t* service = &g_lea_client_service;

    if (!service->started)
        return;

    pthread_mutex_lock(&service->group_lock);
    service->started = false;
    pthread_mutex_unlock(&service->group_lock);

    lea_audio_sink_cleanup();
    lea_audio_source_cleanup();
    bt_sal_lea_cleanup();
}

static bool lea_client_message_prehandle(lea_client_state_machine_t* leas_sm,
    lea_client_msg_t* event)
{
    lea_client_service_t* service = &g_lea_client_service;

    switch (event->event) {
    case STACK_EVENT_STREAM_STARTED: {
        lea_audio_stream_t* audio_stream = (lea_audio_stream_t*)event->data.data;
        lea_client_group_t* group;
        lea_offload_config_t offload = { 0 };
        uint8_t param[sizeof(lea_offload_config_t)];
        size_t size;
        bool ret;

        memcpy(&audio_stream->addr, &event->data.addr, sizeof(bt_address_t));
        audio_stream->started = true;
        audio_stream = lea_client_update_stream(audio_stream);
        if (!audio_stream) {
            break;
        }

        lea_codec_set_config(audio_stream);
        if (!service->offloading) {
            break;
        }

        group = find_group_by_addr(&event->data.addr);
        if (!group) {
            BT_LOGE("%s,  group not exist", __func__);
            break;
        }

        pthread_mutex_lock(&service->group_lock);
        ret = check_group_completed_by_state(group->group_id, LEA_ASCS_OP_STREAMING);
        pthread_mutex_unlock(&service->group_lock);
        if (!ret) {
            BT_LOGD("%s, addr:%s group streamming not completed", __func__, bt_addr_str(&event->data.addr));
            return false;
        }

        BT_LOGD("%s,  group_id:0x%0x, stream_id:0x%08x started", __func__, group->group_id, audio_stream->stream_id);
        lea_codec_get_offload_config(&offload);
        offload.initiator = true;
        ret = lea_offload_start_builder(&offload, param, &size);
        if (!ret) {
            BT_LOGE("failed,  lea_offload_start_builder failed");
            break;
        }

        event->event = OFFLOAD_START_REQ;
        free(event->data.data);
        event->data.data = malloc(size);
        memcpy(event->data.data, param, size);
        event->data.size = size;
        break;
    }
    case STACK_EVENT_STREAM_STOPPED: {
        lea_offload_config_t offload = { 0 };
        uint8_t param[sizeof(lea_offload_config_t)];
        lea_audio_stream_t* stream;
        lea_client_msg_t* msg;
        size_t size;
        bool ret;

        if (!service->offloading) {
            break;
        }

        lea_codec_get_offload_config(&offload);
        ret = lea_offload_stop_builder(&offload, param, &size);
        if (!ret) {
            BT_LOGE("failed, lea_offload_stop_builder");
            break;
        }

        stream = lea_client_find_stream(event->data.valueint1);
        if (stream) {
            lea_codec_unset_config(stream->is_source);
        }

        msg = lea_client_msg_new_ext(OFFLOAD_STOP_REQ, &event->data.addr, param, size);
        if (!msg) {
            BT_LOGE("failed, %s lea_client_msg_new_ext", __func__);
            break;
        }
        lea_client_send_message(msg);
        break;
    }
    default:
        break;
    }

    return true;
}

static void lea_client_process_message(void* data)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_msg_t* msg = (lea_client_msg_t*)data;

    switch (msg->event) {
    case STARTUP:
        client_startup(msg->data.cb);
        break;
    case SHUTDOWN:
        lea_client_do_shutdown();
        break;
    case STACK_EVENT_STACK_STATE: {
        lea_client_notify_stack_state_changed(msg->data.valueint1);
        break;
    }
    default: {
        bool dispatch;
        lea_client_state_machine_t* leasm;

        pthread_mutex_lock(&service->group_lock);
        leasm = get_state_machine(&msg->data.addr);
        if (!leasm) {
            pthread_mutex_unlock(&service->group_lock);
            BT_LOGE("%s, event:%d drop, leasm null", __func__, msg->event);
            break;
        }

        dispatch = lea_client_message_prehandle(leasm, msg);
        if (!dispatch) {
            pthread_mutex_unlock(&service->group_lock);
            BT_LOGE("%s, event:%d not dispatch", __func__, msg->event);
            break;
        }

        lea_client_state_machine_dispatch(leasm, msg);
        pthread_mutex_unlock(&service->group_lock);
        break;
    }
    }

    lea_client_msg_destory(msg);
}

bt_status_t lea_client_send_message(lea_client_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_client_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_send_event(bt_address_t* addr, lea_client_event_t evt)
{
    lea_client_msg_t* msg = lea_client_msg_new(evt, addr);

    if (!msg)
        return BT_STATUS_NOMEM;

    return lea_client_send_message(msg);
}

static void lea_csip_process_message(void* data)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_csip_msg_t* msg = (lea_csip_msg_t*)data;

    switch (msg->event) {
    case STACK_EVENT_CSIP_CS_SIRK: {
        break;
    }
    case STACK_EVENT_CSIP_CS_SIZE: {
        break;
    }
    case STACK_EVENT_CSIP_CS_CREATED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (group) {
            BT_LOGE("%s, group exist", __func__);
            return;
        }

        lea_client_group_new(msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_CSIP_CS_SIZE_UPDATED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (!group) {
            BT_LOGE("%s, group not found", __func__);
            return;
        }

        pthread_mutex_lock(&service->group_lock);
        group->cs_size = msg->event_data.valueint8;
        pthread_mutex_unlock(&service->group_lock);
        break;
    }
    case STACK_EVENT_CSIP_CS_DELETED: {
        lea_client_group_delete(msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_CSIP_CS_LOCKED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (!group) {
            BT_LOGE("%s, group not found", __func__);
            return;
        }
        LEAC_CALLBACK_FOREACH(service->callbacks, client_group_lock_cb, group->group_id, msg->event_data.valueint8);
        break;
    }
    case STACK_EVENT_CSIP_CS_UNLOCKED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (!group) {
            BT_LOGE("%s, group not found", __func__);
            return;
        }
        LEAC_CALLBACK_FOREACH(service->callbacks, client_group_unlock_cb, group->group_id, msg->event_data.valueint8);
        break;
    }
    case STACK_EVENT_CSIP_CS_ORDERED_ACCESS: {
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_LOCKED: {
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_UNLOCKED: {
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_RANK: {
        lea_client_device_t* device;

        device = find_device_by_addr(&msg->addr);
        if (!device) {
            BT_LOGE("%s, device not found", __func__);
            return;
        }

        pthread_mutex_lock(&device->device_lock);
        device->cs_rank = msg->event_data.valueint8;
        pthread_mutex_unlock(&device->device_lock);
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_DISCOVERED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (!group) {
            BT_LOGE("%s, group not found", __func__);
            return;
        }

        LEAC_CALLBACK_FOREACH(service->callbacks, client_group_member_discovered_cb, group->group_id, &msg->addr);
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_ADD: {
        lea_client_group_t* src_group;
        lea_client_group_t* des_group;

        src_group = find_group_by_id(LEA_CLIENT_GROUP_ID_DEFAULT);
        if (!src_group) {
            BT_LOGE("%s, src_group not found", __func__);
            return;
        }

        des_group = find_group_by_sirk(msg->event_data.dataarry);
        if (!des_group) {
            BT_LOGE("%s, des_group not found", __func__);
            return;
        }

        group_move_member(src_group, des_group, &msg->addr);
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_REMOVED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (!group) {
            BT_LOGE("%s, group not found", __func__);
            return;
        }

        group_remove_member(group->group_id, &msg->addr);
        break;
    }
    case STACK_EVENT_CSIP_MEMBER_DISCOVERY_TERMINATED: {
        lea_client_group_t* group;

        group = find_group_by_sirk(msg->event_data.dataarry);
        if (!group) {
            BT_LOGE("%s, group not found", __func__);
            return;
        }

        LEAC_CALLBACK_FOREACH(service->callbacks, client_group_discovery_stop_cb, group->group_id);
        break;
    }
    default: {
        BT_LOGE("Idle: Unexpected stack event:%d", msg->event);
        break;
    }
    }
    lea_csip_msg_destory(msg);
}

static bt_status_t lea_csip_send_message(lea_csip_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_csip_process_message, msg);

    return BT_STATUS_SUCCESS;
}

static void streams_send_message(bool is_source, lea_client_event_t event)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_t* list = service->leac_streams;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;
    lea_client_msg_t* msg;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        if (stream->started && (stream->is_source == is_source)) {
            msg = lea_client_msg_new(event, &stream->addr);
            if (!msg)
                return;

            msg->data.valueint1 = stream->stream_id;
            lea_client_send_message(msg);
        }
    }
}

static void on_lea_sink_audio_suspend(void)
{
    // todo suspend
    BT_LOGD("%s", __func__);
}

static void on_lea_sink_audio_resume(void)
{
    // todo suspend
    BT_LOGD("%s", __func__);
}

static void on_lea_sink_meatadata_updated(void)
{
    streams_send_message(false, STACK_EVENT_METADATA_UPDATED);
}

static void on_lea_source_audio_suspend(void)
{
    // todo suspend
    BT_LOGD("%s", __func__);
}

static void on_lea_source_audio_resume(void)
{
    // todo suspend
    BT_LOGD("%s", __func__);
}

static void on_lea_source_meatadata_updated(void)
{
    streams_send_message(true, STACK_EVENT_METADATA_UPDATED);
}

static void lea_audio_send_data(lea_audio_stream_t* stream, uint8_t* buffer, uint16_t length)
{
    lea_send_iso_data_t* iso_pkt;

    iso_pkt = bt_sal_lea_alloc_send_buffer(stream->sdu_size, stream->iso_handle);
    memcpy(iso_pkt->sdu, buffer, length);
    iso_pkt->sdu_length = length;

    bt_sal_lea_send_iso_data(iso_pkt);
}

static void on_lea_source_audio_send(uint8_t* buffer, uint16_t length)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_t* list = service->leac_streams;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        // todo mix for many streams?
        if (stream->started && !stream->is_source) {
            lea_audio_send_data(stream, buffer, length);
        }
    }
}

static bt_status_t lea_client_init(void)
{
    pthread_mutexattr_t attr;
    lea_client_service_t* service = &g_lea_client_service;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->group_lock, &attr);
    pthread_mutex_init(&service->stream_lock, &attr);

    service->max_connections = CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_CONNECTIONS;
    service->leac_groups = bt_list_new((bt_list_free_cb_t)group_delete_cb);
    service->leac_streams = bt_list_new(NULL);
    service->callbacks = bt_callbacks_list_new(2);
    service->index_allocator = index_allocator_create(CONFIG_BLUETOOTH_LEAUDIO_CLIENT_MAX_ALLOC_NUMBER);
    service->index_allocator->id_next = LEA_CLIENT_GROUP_ID_DEFAULT + 1;

    BT_LOGD("%s", __func__);

    return BT_STATUS_SUCCESS;
}

static void lea_client_cleanup(void)
{
    lea_client_service_t* service = &g_lea_client_service;
    BT_LOGD("%s", __func__);

    pthread_mutex_lock(&service->group_lock);
    bt_list_free(service->leac_groups);
    service->leac_groups = NULL;
    pthread_mutex_unlock(&service->group_lock);

    pthread_mutex_lock(&service->stream_lock);
    bt_list_free(service->leac_streams);
    service->leac_streams = NULL;
    pthread_mutex_unlock(&service->stream_lock);

    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;

    index_allocator_delete(&service->index_allocator);

    pthread_mutex_destroy(&service->stream_lock);
    pthread_mutex_destroy(&service->group_lock);
}

static void client_startup(void* data)
{
    bt_status_t ret;
    lea_client_service_t* service = &g_lea_client_service;
    profile_on_startup_t on_startup = (profile_on_startup_t)data;

    pthread_mutex_lock(&service->group_lock);

    ret = bt_sal_lea_init();
    if (ret != BT_STATUS_SUCCESS) {
        goto end;
    }

    ret = lea_audio_sink_init(service->offloading);
    if (ret != BT_STATUS_SUCCESS) {
        goto end;
    }

    ret = lea_audio_source_init(service->offloading);
    if (ret != BT_STATUS_SUCCESS) {
        goto end;
    }

    lea_client_group_new(NULL);
    service->started = true;
    on_startup(PROFILE_LEAUDIO_CLIENT, true);
    ret = BT_STATUS_SUCCESS;

end:
    pthread_mutex_unlock(&service->group_lock);
}

static bt_status_t lea_client_startup(profile_on_startup_t cb)
{
    lea_client_service_t* service = &g_lea_client_service;

    BT_LOGD("%s", __func__);
    pthread_mutex_lock(&service->group_lock);
    if (service->started) {
        pthread_mutex_unlock(&service->group_lock);
        return BT_STATUS_SUCCESS;
    }
    pthread_mutex_unlock(&service->group_lock);

    lea_client_msg_t* msg = lea_client_msg_new(STARTUP, NULL);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.cb = cb;
    return lea_client_send_message(msg);
}

static bt_status_t lea_client_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);

    return lea_client_send_event(NULL, SHUTDOWN);
}

static void lea_client_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    case PROFILE_EVT_LEA_OFFLOADING:
        g_lea_client_service.offloading = msg->data.valuebool;
        break;

    default:
        break;
    }
}

static void* lea_client_register_callbacks(void* remote, const lea_client_callbacks_t* callbacks)
{
    lea_client_service_t* service = &g_lea_client_service;

    if (!service->started)
        return NULL;

    return bt_remote_callbacks_register(service->callbacks,
        remote, (void*)callbacks);
}

static bool lea_client_unregister_callbacks(void** remote, void* cookie)
{
    lea_client_service_t* service = &g_lea_client_service;

    if (!service->started)
        return false;

    return bt_remote_callbacks_unregister(service->callbacks,
        remote, cookie);
}

static profile_connection_state_t lea_client_get_connection_state(bt_address_t* addr)
{
    lea_client_device_t* device;
    profile_connection_state_t conn_state;

    device = find_device_by_addr(addr);
    if (!device)
        return PROFILE_STATE_DISCONNECTED;

    pthread_mutex_lock(&device->device_lock);
    conn_state = device->state;
    pthread_mutex_unlock(&device->device_lock);

    return conn_state;
}

static bt_status_t lea_client_connect_device(bt_address_t* addr)
{
    CHECK_ENABLED();
    return lea_client_send_event(addr, CONNECT_DEVICE);
}

static bt_status_t lea_client_get_group_id(bt_address_t* addr, uint32_t* group_id)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;

    CHECK_ENABLED();

    group = find_group_by_addr(addr);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&service->group_lock);
    *group_id = group->group_id;
    pthread_mutex_unlock(&service->group_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t get_ases_streams_id_form_group(uint32_t group_id, uint8_t* num, uint32_t* stream_ids)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_node_t* cnode;
    bt_list_t* clist;
    lea_client_device_t* device;
    lea_client_group_t* group;
    int index, cnt;

    *num = 0;
    cnt = 0;
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    clist = group->devices;
    pthread_mutex_lock(&service->group_lock);
    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        device = bt_list_node(cnode);
        for (index = 0; index < device->ase_number; index++) {
            stream_ids[cnt++] = device->ase[index].stream_id;
        }
    }
    *num = cnt;
    pthread_mutex_unlock(&service->group_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t get_ases_streams_id_form_context(uint32_t group_id, uint8_t* num, uint32_t* stream_ids)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_node_t* cnode;
    bt_list_t* clist;
    lea_client_device_t* device;
    lea_client_group_t* group;
    int index, cnt;

    *num = 0;
    cnt = 0;
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    clist = group->devices;
    pthread_mutex_lock(&service->group_lock);
    for (cnode = bt_list_head(clist); cnode != NULL; cnode = bt_list_next(clist, cnode)) {
        device = bt_list_node(cnode);
        for (index = 0; index < device->ase_number; index++) {
            if (device->ase[index].active) {
                stream_ids[cnt++] = device->ase[index].stream_id;
            }
        }
    }
    *num = cnt;
    pthread_mutex_unlock(&service->group_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t get_ases_streams_id_from_addr(uint32_t group_id, bt_address_t* addr, uint8_t* num, uint32_t* stream_ids)
{
    lea_client_device_t* device;
    int index;

    device = find_device_by_groupid_addr(group_id, addr);
    if (!device) {
        BT_LOGE("%s, device not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&device->device_lock);
    for (index = 0; index < device->ase_number; index++) {
        stream_ids[index] = device->ase[index].stream_id;
    }
    *num = index;
    pthread_mutex_unlock(&device->device_lock);

    return BT_STATUS_SUCCESS;
}

static bool lea_client_ucc_source_context_is_valid(uint16_t context, lea_client_device_t* device)
{
    // Check Available_Audio_Contexts and Supported_Audio_Contexts bit set
    if ((LEA_BIT(context) & device->source_avaliable_ctx) && (LEA_BIT(context) & device->source_supported_ctx)) {
        return true;
    }

    // Check Available_Audio_Contexts and Supported_Audio_Contexts  «Unspecified» bit set
    if (!(LEA_BIT(context) & device->source_supported_ctx) && (device->source_avaliable_ctx & LEA_BIT(ADPT_LEA_CTX_ID_UNSPECIFIED)) && (device->source_supported_ctx & LEA_BIT(ADPT_LEA_CTX_ID_UNSPECIFIED))) {
        return true;
    }

    return false;
}

static bool lea_client_ucc_sink_context_is_valid(uint16_t context, lea_client_device_t* device)
{
    // Check Available_Audio_Contexts and Supported_Audio_Contexts bit set
    if ((LEA_BIT(context) & device->sink_avaliable_ctx) && (LEA_BIT(context) & device->sink_supported_ctx)) {
        return true;
    }

    // Check Available_Audio_Contexts and Supported_Audio_Contexts  «Unspecified» bit set
    if (!(LEA_BIT(context) & device->sink_supported_ctx) && (device->sink_avaliable_ctx & LEA_BIT(ADPT_LEA_CTX_ID_UNSPECIFIED)) && (device->sink_supported_ctx & LEA_BIT(ADPT_LEA_CTX_ID_UNSPECIFIED))) {
        return true;
    }

    return false;
}

static uint16_t lea_client_get_initiator_contexts(uint8_t context)
{
    int num;
    uint16_t contexts;

    num = sizeof(g_lc3_local_prefer_configs) / sizeof(g_lc3_local_prefer_configs[0]);
    for (int index = 0; index < num; index++) {
        contexts = g_lc3_local_prefer_configs[index].contexts;
        if (contexts & LEA_BIT(context)) {
            return contexts;
        }
    }

    return ADPT_LEA_CONTEXT_TYPE_PROHIBITED;
}

static const lea_lc3_prefer_config* lea_client_get_initiator_lc3_config(uint8_t context)
{
    int num;
    const lea_lc3_prefer_config* config;

    num = sizeof(g_lc3_local_prefer_configs) / sizeof(g_lc3_local_prefer_configs[0]);
    for (int index = 0; index < num; index++) {
        config = &g_lc3_local_prefer_configs[index];
        if (config->contexts & LEA_BIT(context)) {
            return config;
        }
    }

    return NULL;
}

static bt_status_t connect_audio_internal(lea_client_group_t* group, lea_client_device_t* device)
{
    profile_connection_state_t state;
    lea_client_msg_t* msg;
    uint16_t local_contexts;

    pthread_mutex_lock(&device->device_lock);
    state = device->state;
    pthread_mutex_unlock(&device->device_lock);
    if (state != PROFILE_STATE_CONNECTED) {
        BT_LOGE("%s, device no connected", __func__);
        return BT_STATUS_NO_RESOURCES;
    }

    local_contexts = lea_client_get_initiator_contexts(group->context);
    BT_LOGD("%s, local_contexts:0x%08x", __func__, local_contexts);
    if (!lea_client_ucc_sink_context_is_valid(group->context, device) && !lea_client_ucc_source_context_is_valid(group->context, device)) {
        BT_LOGE("%s, local_contexts:0x%08x not avaliable", __func__, local_contexts);
        return BT_STATUS_NOT_SUPPORTED;
    }

    msg = lea_client_msg_new(CONNECT_AUDIO, &device->addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = group->group_id;
    return lea_client_send_message(msg);
}

static bt_status_t lea_client_connect_audio(bt_address_t* addr, uint8_t context)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;
    lea_client_device_t* device;

    CHECK_ENABLED();

    if (context >= ADPT_LEA_CTX_ID_NUMBER) {
        BT_LOGE("%s, context(%d) not support", __func__, context);
        return BT_STATUS_NOT_SUPPORTED;
    }

    group = find_group_by_addr(addr);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    device = find_device_by_group_addr(group, addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&service->group_lock);
    group->context = context;
    pthread_mutex_unlock(&service->group_lock);

    return connect_audio_internal(group, device);
}

static bt_status_t lea_client_disconnect_device(bt_address_t* addr)
{
    CHECK_ENABLED();
    profile_connection_state_t state = lea_client_get_connection_state(addr);
    if (state == PROFILE_STATE_DISCONNECTED || state == PROFILE_STATE_DISCONNECTING)
        return BT_STATUS_FAIL;

    return lea_client_send_event(addr, DISCONNECT_DEVICE);
}

static bt_status_t disconnect_audio_internal(uint32_t group_id, bt_address_t* addr)
{
    lea_client_msg_t* msg = lea_client_msg_new(DISCONNECT_AUDIO, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = group_id;
    return lea_client_send_message(msg);
}

static bt_status_t lea_client_disconnect_audio(bt_address_t* addr)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_addr(addr);
    if (!group) {
        return BT_STATUS_NOT_FOUND;
    }

    return disconnect_audio_internal(group->group_id, addr);
}

bt_status_t lea_csip_read_sirk(bt_address_t* addr)
{
    CHECK_ENABLED();
    return bt_sal_lea_csip_read_sirk(addr);
}

bt_status_t lea_csip_read_cs_size(bt_address_t* addr)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_addr(addr);
    if (!group) {
        return BT_STATUS_NOT_FOUND;
    }

    return bt_sal_lea_csip_read_cs_size(addr);
}

bt_status_t lea_csip_read_member_lock(bt_address_t* addr)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_addr(addr);
    if (!group) {
        return BT_STATUS_NOT_FOUND;
    }
    return bt_sal_lea_csip_read_member_lock(addr);
}

bt_status_t lea_csip_read_member_rank(bt_address_t* addr)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_addr(addr);
    if (!group) {
        return BT_STATUS_NOT_FOUND;
    }

    return bt_sal_lea_csip_read_member_rank(addr);
}

static bt_status_t lea_client_discovery_member_start(uint32_t group_id)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_id(group_id);
    if (!group) {
        return BT_STATUS_NOT_FOUND;
    }

    return bt_sal_lea_csip_coordinated_set_discovery_member_start(group->sirk);
}

static bt_status_t lea_client_discovery_member_stop(uint32_t group_id)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_id(group_id);
    if (!group) {
        return BT_STATUS_NOT_FOUND;
    }

    return bt_sal_lea_csip_coordinated_set_discovery_member_stop(group->sirk);
}

static bt_status_t lea_client_group_add_member(uint32_t group_id, bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_device_t* device;
    lea_client_state_machine_t* leasm;

    CHECK_ENABLED();
    device = find_device_by_groupid_addr(group_id, addr);
    if (device) {
        goto end;
    }

    leasm = lea_client_state_machine_new(addr, (void*)service);
    if (!leasm) {
        BT_LOGE("Create state machine failed");
        return BT_STATUS_NOMEM;
    }

    device = lea_client_device_new(addr, leasm);
    group_add_member(group_id, device);

end:
    LEAC_CALLBACK_FOREACH(service->callbacks, client_group_member_added_cb, group_id, addr);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_group_remove_member(uint32_t group_id, bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_status_t ret;
    CHECK_ENABLED();

    ret = group_remove_member(group_id, addr);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    LEAC_CALLBACK_FOREACH(service->callbacks, client_group_member_removed_cb, group_id, addr);
    return BT_STATUS_SUCCESS;
}

static void group_connect_audio_cb(void* data, void* context)
{
    lea_client_device_t* device = (lea_client_device_t*)data;

    connect_audio_internal((lea_client_group_t*)context, device);
}

static bt_status_t lea_client_group_connect_audio(uint32_t group_id, uint8_t context)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }
    pthread_mutex_lock(&service->group_lock);
    group->context = context;
    pthread_mutex_unlock(&service->group_lock);

    bt_list_foreach(group->devices, group_connect_audio_cb, group);
    return BT_STATUS_SUCCESS;
}

static void group_disconnect_audio_cb(void* data, void* context)
{
    lea_client_device_t* device = (lea_client_device_t*)data;
    lea_client_group_t* group = (lea_client_group_t*)context;

    disconnect_audio_internal(group->group_id, &device->addr);
}

static bt_status_t lea_client_group_disconnect_audio(uint32_t group_id)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    bt_list_foreach(group->devices, group_disconnect_audio_cb, group);
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_group_lock(uint32_t group_id)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    return bt_sal_lea_csip_coordinated_set_lock_request(group->sirk);
}

static bt_status_t lea_client_group_unlock(uint32_t group_id)
{
    lea_client_group_t* group;

    CHECK_ENABLED();
    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    return bt_sal_lea_csip_coordinated_set_lock_release(group->sirk);
}

static const void* get_leac_profile_interface(void)
{
    return &LEAClientInterface;
}

static int lea_client_dump(void)
{
    printf("impl hfp hf dump");
    return 0;
}

static bool lea_client_stream_cmp(void* audio_stream, void* stream_id)
{
    return ((lea_audio_stream_t*)audio_stream)->stream_id == *((uint32_t*)stream_id);
}

static int lea_client_get_state(void)
{
    return 1;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

lea_audio_stream_t* lea_client_add_stream(
    uint32_t stream_id, bt_address_t* addr)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_audio_stream_t* audio_stream;

    audio_stream = malloc(sizeof(lea_audio_stream_t));
    if (!audio_stream) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }

    audio_stream->stream_id = stream_id;
    audio_stream->started = false;
    audio_stream->is_source = bt_sal_lea_is_source_stream(stream_id);
    memcpy(&audio_stream->addr, addr, sizeof(bt_address_t));

    pthread_mutex_lock(&service->stream_lock);
    bt_list_add_tail(service->leac_streams, audio_stream);
    pthread_mutex_unlock(&service->stream_lock);

    return audio_stream;
}

lea_audio_stream_t* lea_client_find_stream(uint32_t stream_id)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_audio_stream_t* stream;

    pthread_mutex_lock(&service->stream_lock);
    stream = bt_list_find(service->leac_streams, lea_client_stream_cmp, &stream_id);
    pthread_mutex_unlock(&service->stream_lock);

    return stream;
}

lea_audio_stream_t* lea_client_update_stream(lea_audio_stream_t* stream)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_audio_stream_t* local_stream;
    lea_client_group_t* group;

    group = find_group_by_addr(&stream->addr);
    if (!group) {
        BT_LOGE("%s, addr:%s, group_id:0x%0x, is_source:%d, group not exist", __func__, bt_addr_str(&stream->addr),
            stream->group_id, stream->is_source)
        return NULL;
    }
    pthread_mutex_lock(&service->stream_lock);
    local_stream = bt_list_find(service->leac_streams, lea_client_stream_cmp, &stream->stream_id);
    if (!local_stream) {
        pthread_mutex_unlock(&service->stream_lock);
        BT_LOGE("%s, addr:%s, group_id:0x%0x, is_source:%d, local_stream not exist", __func__, bt_addr_str(&stream->addr),
            stream->group_id, stream->is_source)
        return NULL;
    }

    stream->group_id = group->group_id;
    memcpy(local_stream, stream, sizeof(lea_audio_stream_t));
    pthread_mutex_unlock(&service->stream_lock);
    BT_LOGD("%s addr:%s, group_id:0x%0x, is_source:%d, started:%d", __func__, bt_addr_str(&local_stream->addr),
        local_stream->group_id, local_stream->is_source,
        local_stream->started);

    return local_stream;
}

void lea_client_remove_stream(uint32_t stream_id)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_audio_stream_t* audio_stream;

    pthread_mutex_lock(&service->stream_lock);
    audio_stream = bt_list_find(service->leac_streams, lea_client_stream_cmp, &stream_id);
    bt_list_remove(service->leac_streams, audio_stream);
    pthread_mutex_unlock(&service->stream_lock);
}

void lea_client_remove_streams()
{
    lea_client_service_t* service = &g_lea_client_service;

    pthread_mutex_lock(&service->stream_lock);
    bt_list_clear(service->leac_streams);
    pthread_mutex_unlock(&service->stream_lock);
}

void lea_client_notify_stack_state_changed(lea_client_stack_state_t
        enabled)
{
    lea_client_service_t* service = &g_lea_client_service;

    LEAC_CALLBACK_FOREACH(service->callbacks, client_stack_state_cb, enabled);
}

void lea_client_notify_connection_state_changed(bt_address_t* addr,
    profile_connection_state_t state)
{
    lea_client_service_t* service = &g_lea_client_service;
    lea_client_device_t* device;

    device = find_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device no exist", __func__);
        return;
    }

    pthread_mutex_lock(&device->device_lock);
    device->state = state;
    pthread_mutex_unlock(&device->device_lock);

    LEAC_CALLBACK_FOREACH(service->callbacks, client_connection_state_cb, state, addr);
}

static bool leac_client_lc3_duation_checked(const lea_client_capability_t* pac, const lea_lc3_config_t* lc3_config)
{
    bool duration_10 = lc3_config->duration;
    bool ret = false;

    if (duration_10) {
        if (pac->codec_cap.durations & ADPT_LEA_PREFERRED_FRAME_DURATION_10) {
            ret = true;
        } else if (pac->codec_cap.durations & ADPT_LEA_SUPPORTED_FRAME_DURATION_10) {
            ret = true;
        }
    } else {
        if (pac->codec_cap.durations & ADPT_LEA_PREFERRED_FRAME_DURATION_7_5) {
            ret = true;
        } else if (pac->codec_cap.durations & ADPT_LEA_SUPPORTED_FRAME_DURATION_7_5) {
            ret = true;
        }
    }

    return ret;
}

static bool leac_client_lc3_frequency_checked(const lea_client_capability_t* pac, const lea_lc3_config_t* lc3_config)
{
    bool ret = false;

    if (lc3_config->frequency < 1) {
        BT_LOGE("%s, invalid frequency(%d)", __func__, lc3_config->frequency);
        return false;
    }

    if (pac->codec_cap.frequencies & (1 << (lc3_config->frequency - 1))) {
        ret = true;
    }

    return ret;
}

static bool leac_client_lc3_octets_checked(const lea_client_capability_t* pac, const lea_lc3_config_t* lc3_config)
{
    bool ret = false;

    if ((lc3_config->octets >= pac->codec_cap.frame_octets_min) && (lc3_config->octets <= pac->codec_cap.frame_octets_max)) {
        ret = true;
    }

    return ret;
}

static lea_lc3_set_id_t lea_client_get_prefer_lc3_set_id_from_pac(lea_client_capability_t* pac, uint8_t context)
{
    int index;
    uint8_t* set_ids;
    lea_lc3_set_id_t set_id;
    const lea_lc3_config_t* lc3_config;
    const lea_lc3_prefer_config* local_config;

    local_config = lea_client_get_initiator_lc3_config(context);
    set_ids = local_config->set_10_ids;
    set_id = ADPT_LEA_LC3_SET_UNKNOWN;

    for (index = 0; index < local_config->set_number; index++) {
        set_id = set_ids[index];
        lc3_config = &g_lea_lc3_configs[set_id];

        if (leac_client_lc3_frequency_checked(pac, lc3_config) && leac_client_lc3_octets_checked(pac, lc3_config) && leac_client_lc3_duation_checked(pac, lc3_config)) {
            break;
        }

        set_id = ADPT_LEA_LC3_SET_UNKNOWN;
    }

    return set_id;
}

static void lea_client_dump_pac(lea_client_device_t* device)
{
    lea_client_capability_t* pac;

    for (int pac_index = 0; pac_index < device->pac_number; pac_index++) {
        pac = &device->pac[pac_index];
        BT_LOGD("pac id:0x%08x", pac->pac_id);
        for (int md_index = 0; md_index < pac->metadata_number; md_index++) {
            BT_LOGD("pac md type:%d", pac->metadata_value[md_index].type);
            lib_dumpbuffer("pac md value", pac->metadata_value[md_index].extended_metadata, sizeof(pac->metadata_value[md_index].extended_metadata));
        }
    }
}

static lea_lc3_set_id_t lea_client_get_prefer_lc3_set_id_from_ase(uint8_t context, lea_client_device_t* device, lea_client_endpoint_t* ase)
{
    lea_client_capability_t* pac;
    uint32_t preferred_contexts;
    lea_lc3_set_id_t lc3_set_id;

    for (int pac_index = 0; pac_index < device->pac_number; pac_index++) {
        pac = &device->pac[pac_index];
        preferred_contexts = ADPT_LEA_CONTEXT_TYPE_PROHIBITED;

        if (ase->is_source != pac->is_source) {
            continue;
        }

        for (int md_index = 0; md_index < pac->metadata_number; md_index++) {
            if (pac->metadata_value[md_index].type == ADPT_LEA_METADATA_PREFERRED_AUDIO_CONTEXTS) {
                preferred_contexts = pac->metadata_value[md_index].preferred_contexts;
                break;
            }
        }

        if (preferred_contexts & LEA_BIT(context)) {
            lc3_set_id = lea_client_get_prefer_lc3_set_id_from_pac(pac, context);
            if (lc3_set_id != ADPT_LEA_LC3_SET_UNKNOWN) {
                return lc3_set_id;
            }
        }
    }

    return ADPT_LEA_LC3_SET_UNKNOWN;
}

static lea_lc3_set_id_t lea_client_get_avaliable_lc3_set_id_from_ase(uint8_t context, lea_client_device_t* device, lea_client_endpoint_t* ase)
{
    lea_client_capability_t* pac;
    uint32_t avaliable_contexts;
    lea_lc3_set_id_t lc3_set_id;

    if (ase->is_source) {
        avaliable_contexts = device->source_avaliable_ctx;
    } else {
        avaliable_contexts = device->sink_avaliable_ctx;
    }

    for (int pac_index = 0; pac_index < device->pac_number; pac_index++) {
        pac = &device->pac[pac_index];

        if (ase->is_source != pac->is_source) {
            continue;
        }

        if (avaliable_contexts & LEA_BIT(context)) {
            lc3_set_id = lea_client_get_prefer_lc3_set_id_from_pac(pac, context);
            if (lc3_set_id != ADPT_LEA_LC3_SET_UNKNOWN) {
                return lc3_set_id;
            }
        }
    }

    return ADPT_LEA_LC3_SET_UNKNOWN;
}

static bt_status_t lea_client_ucc_get_prefer_stream(lea_client_group_t* group, lea_client_device_t* device, lea_client_endpoint_t* endpoint, lea_audio_stream_t* stream)
{
    lea_lc3_set_id_t lc3_set_id;

    lc3_set_id = lea_client_get_prefer_lc3_set_id_from_ase(group->context, device, endpoint);
    if (lc3_set_id == ADPT_LEA_LC3_SET_UNKNOWN) {
        BT_LOGD("%s, try lea_client_get_avaliable_pac", __func__);
        lc3_set_id = lea_client_get_avaliable_lc3_set_id_from_ase(group->context, device, endpoint);
        if (lc3_set_id == ADPT_LEA_LC3_SET_UNKNOWN) {
            BT_LOGE("%s, lea_client_get_avaliable_pac fail", __func__);
            return BT_STATUS_FAIL;
        }
    }
    BT_LOGD("%s, lc3_set_id:%d", __func__, lc3_set_id);

    memcpy(&stream->addr, &device->addr, sizeof(bt_address_t));
    stream->stream_id = endpoint->stream_id;
    stream->target_latency = ADPT_LEA_ASE_TARGET_BALANCED;
    stream->target_phy = ADPT_LEA_ASE_TARGET_PHY_2M;

    stream->codec_cfg.codec_id.format = 0x06; // LC3
    stream->codec_cfg.frequency = g_lea_lc3_configs[lc3_set_id].frequency;
    stream->codec_cfg.duration = g_lea_lc3_configs[lc3_set_id].duration;
    stream->codec_cfg.octets = g_lea_lc3_configs[lc3_set_id].octets;

    // todo prefer blocks and allocation
    stream->codec_cfg.blocks = 1;
    stream->codec_cfg.allocation = 0x01;

    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_client_alloc_cis_id(uint8_t* cis_id)
{
    lea_client_service_t* service = &g_lea_client_service;
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
    lea_client_service_t* service = &g_lea_client_service;

    index_free(service->index_allocator, cis_id);
}

static bt_status_t lea_client_get_stream_id(uint32_t group_id, bt_address_t* addr, lea_client_endpoint_t* endpoint,
    uint8_t cis_id, uint32_t* stream_id)
{
    CHECK_ENABLED();

    *stream_id = endpoint->stream_id;
    if (*stream_id > 0) {
        return BT_STATUS_SUCCESS;
    }

    return bt_sal_lea_alloc_stream_id(group_id, cis_id, endpoint->ase_id, endpoint->is_source, stream_id);
}

static bool lea_client_filter_ase_from_context(int context, bool is_source)
{
    switch (context) {
    case ADPT_LEA_CTX_ID_UNSPECIFIED:
        return false;
    case ADPT_LEA_CTX_ID_CONVERSATIONAL:
        return true;
    case ADPT_LEA_CTX_ID_MEDIA:
    case ADPT_LEA_CTX_ID_GAME:
    case ADPT_LEA_CTX_ID_INSTRUCTIONAL:
    case ADPT_LEA_CTX_ID_SOUND_EFFECTS:
    case ADPT_LEA_CTX_ID_NOTIFICATIONS:
    case ADPT_LEA_CTX_ID_RINGTONE:
    case ADPT_LEA_CTX_ID_ALERTS:
    case ADPT_LEA_CTX_ID_EMERGENCY_ALARM:
        return !is_source;
    case ADPT_LEA_CTX_ID_VOICE_ASSISTANTS:
        return is_source;
    }

    return false;
}

bt_status_t lea_client_ucc_add_streams(uint32_t group_id, bt_address_t* addr)
{
    lea_client_device_t* device;
    lea_audio_stream_t stream;
    int index;
    uint8_t cis_id;
    bt_status_t ret;
    uint32_t stream_id;
    lea_client_group_t* group;
    bool opq = true;

    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, device not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    device = find_device_by_group_addr(group, addr);
    if (!device) {
        BT_LOGE("%s, device not found", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&device->device_lock);
    ret = lea_client_alloc_cis_id(&cis_id);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, alloc_cis_id failed", __func__);
        goto end;
    }
    device->cis_id = cis_id;
    lea_client_dump_pac(device);

    for (index = 0; index < device->ase_number; index++) {
        opq = true;
        ret = lea_client_get_stream_id(group_id, addr, &device->ase[index], device->cis_id, &stream_id);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s, get_stream_id failed", __func__);
            goto end;
        }

        device->ase[index].stream_id = stream_id;
        if (lea_client_ucc_get_prefer_stream(group, device, &device->ase[index], &stream) == BT_STATUS_SUCCESS) {
            for (int j = 0; j < index; j++) {
                if (device->ase[j].is_source == device->ase[index].is_source) {
                    opq = false;
                }
            }

            if (!lea_client_filter_ase_from_context(group->context, device->ase[index].is_source)) {
                opq = false;
            }

            if (opq) {
                device->ase[index].active = true;
                bt_sal_lea_ucc_group_add_stream(group_id, &stream);
            }
        }
    }

end:
    pthread_mutex_unlock(&device->device_lock);
    return ret;
}

bt_status_t lea_client_ucc_remove_streams(uint32_t group_id, bt_address_t* addr)
{
    uint8_t number;
    bt_status_t ret;
    uint32_t stream_ids[LEA_CLIENT_MAX_STREAM_NUM];
    lea_client_device_t* device;

    ret = get_ases_streams_id_from_addr(group_id, addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, device no exist", __func__);
        return ret;
    }

    if (number > LEA_CLIENT_MAX_STREAM_NUM) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
            LEA_CLIENT_MAX_STREAM_NUM);
        return BT_STATUS_NOMEM;
    }

    device = find_device_by_addr(addr);
    if (!device) {
        return BT_STATUS_NOT_FOUND;
    }
    lea_client_free_cis_id(device->cis_id);

    for (int index = 0; index < device->ase_number; index++) {
        device->ase[index].active = false;
    }

    return bt_sal_lea_ucc_group_remove_stream(group_id, number, stream_ids);
}

bt_status_t lea_client_ucc_config_codec(uint32_t group_id, bt_address_t* addr)
{
    uint8_t number;
    uint32_t stream_ids[LEA_CLIENT_MAX_STREAM_NUM];
    bt_status_t ret;

    ret = get_ases_streams_id_from_addr(group_id, addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_ases_streams_id_from_addr failed", __func__);
        return ret;
    }

    if (number > LEA_CLIENT_MAX_STREAM_NUM) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
            LEA_CLIENT_MAX_STREAM_NUM);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_request_codec(group_id, number, stream_ids);
}

bt_status_t lea_client_ucc_config_qos(uint32_t group_id, bt_address_t* addr, uint32_t stream_id)
{
    lea_client_service_t* service = &g_lea_client_service;
    bool completed;
    uint8_t number;
    bt_status_t ret;
    uint32_t stream_ids[LEA_CLIENT_MAX_STREAM_NUM];
    lea_client_device_t* device;
    lea_client_endpoint_t* ase;

    device = find_device_by_groupid_addr(group_id, addr);
    if (!device) {
        return BT_STATUS_NOT_FOUND;
    }

    ase = find_ase_by_device_streamid(device, stream_id);
    if (!ase) {
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&device->device_lock);
    ase->op = LEA_ASCS_OP_QOS;
    pthread_mutex_unlock(&device->device_lock);

    pthread_mutex_lock(&service->group_lock);
    completed = check_group_completed_by_op(group_id, LEA_ASCS_OP_QOS);
    pthread_mutex_unlock(&service->group_lock);
    if (!completed) {
        BT_LOGD("%s, addr:%s group qos not completed", __func__, bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    ret = get_ases_streams_id_form_group(group_id, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_ases_streams_id_form_group failed", __func__);
        return ret;
    }
    BT_LOGD("%s, addr:%s, number:%d", __func__, bt_addr_str(addr), number);

    if (number > LEA_CLIENT_MAX_STREAM_NUM) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
            LEA_CLIENT_MAX_STREAM_NUM);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_request_qos(group_id, number,
        stream_ids);
}

bt_status_t lea_client_ucc_enable(uint32_t group_id, bt_address_t* addr, uint32_t stream_id)
{
    lea_client_service_t* service = &g_lea_client_service;
    uint8_t number;
    bt_status_t ret;
    int index;
    uint32_t stream_ids[LEA_CLIENT_MAX_STREAM_NUM];
    lea_metadata_t metadata[LEA_CLIENT_MAX_STREAM_NUM];
    bool completed;
    lea_client_device_t* device;
    lea_client_endpoint_t* ase;
    lea_client_msg_t* msg;
    lea_client_group_t* group;

    group = find_group_by_id(group_id);
    if (!group) {
        BT_LOGE("%s, group no exist", __func__);
        return BT_STATUS_NOT_FOUND;
    }

    device = find_device_by_group_addr(group, addr);
    if (!device) {
        return BT_STATUS_NOT_FOUND;
    }

    ase = find_ase_by_device_streamid(device, stream_id);
    if (!ase) {
        return BT_STATUS_NOT_FOUND;
    }

    pthread_mutex_lock(&device->device_lock);
    ase->op = LEA_ASCS_OP_ENABLING;
    pthread_mutex_unlock(&device->device_lock);

    pthread_mutex_lock(&service->group_lock);
    completed = check_group_completed_by_op(group_id, LEA_ASCS_OP_ENABLING);
    pthread_mutex_unlock(&service->group_lock);
    if (!completed) {
        BT_LOGD("%s, addr:%s group enabling not completed", __func__, bt_addr_str(addr));
        return BT_STATUS_FAIL;
    }

    ret = get_ases_streams_id_form_context(group_id, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_ases_streams_id_form_group failed", __func__);
        return ret;
    }
    BT_LOGD("%s, addr:%s, number:%d", __func__, bt_addr_str(addr), number);

    if (number > LEA_CLIENT_MAX_STREAM_NUM) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
            LEA_CLIENT_MAX_STREAM_NUM);
        return BT_STATUS_NOMEM;
    }

    for (index = 0; index < number; index++) {
        metadata[index].streaming_contexts = LEA_BIT(group->context);
        metadata[index].type = ADPT_LEA_METADATA_STREAMING_AUDIO_CONTEXTS;
    }

    // barrot stack stream started event come before enabling, here let sm come into started state
    msg = lea_client_msg_new(STACK_EVENT_ASE_ENABLING, addr);
    if (!msg)
        return BT_STATUS_NOMEM;

    msg->data.valueint1 = stream_id;
    msg->data.valueint2 = 0;
    msg->data.valueint3 = group_id;
    lea_client_send_message(msg);

    // todo prefer streams according to context
    return bt_sal_lea_ucc_group_request_enable(group_id, number, stream_ids, metadata);
}

bt_status_t lea_client_ucc_disable(uint32_t group_id, bt_address_t* addr)
{
    uint8_t number;
    uint32_t stream_ids[LEA_CLIENT_MAX_STREAM_NUM];
    bt_status_t ret;

    ret = get_ases_streams_id_from_addr(group_id, addr, &number, stream_ids);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, get_ases_streams_id_from_addr failed", __func__);
        return ret;
    }

    if (number > LEA_CLIENT_MAX_STREAM_NUM) {
        BT_LOGE("%s, stream number(%d) over max(%d)", __func__, number,
            LEA_CLIENT_MAX_STREAM_NUM);
        return BT_STATUS_NOMEM;
    }

    return bt_sal_lea_ucc_group_request_disable(group_id, number,
        stream_ids);
}

bt_status_t lea_client_ucc_started(uint32_t group_id)
{
    lea_client_service_t* service = &g_lea_client_service;
    bt_list_t* list = service->leac_streams;
    lea_audio_stream_t* stream;
    bt_list_node_t* node;
    lea_client_msg_t* msg;
    lea_client_device_t* device;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        stream = bt_list_node(node);
        if (stream->started && (stream->group_id == group_id)) {
            device = find_device_by_addr(&stream->addr);
            if (!device) {
                BT_LOGE("%s, device not exist", __func__);
                continue;
            }

            msg = lea_client_msg_new_ext(STACK_EVENT_STREAM_STARTED, &stream->addr,
                stream, sizeof(lea_audio_stream_t));
            if (!msg)
                return BT_STATUS_NOMEM;

            lea_client_state_machine_dispatch(device->leasm, msg);
            lea_client_msg_destory(msg);
        }
    }

    return BT_STATUS_SUCCESS;
}

void lea_client_on_stack_state_changed(lea_client_stack_state_t enabled)
{
    lea_client_msg_t* msg = lea_client_msg_new(STACK_EVENT_STACK_STATE,
        NULL);
    if (!msg)
        return;

    msg->data.valueint1 = enabled;
    lea_client_send_message(msg);
}

void lea_client_on_connection_state_changed(bt_address_t* addr,
    profile_connection_state_t state)
{
    lea_client_msg_t* msg = lea_client_msg_new(STACK_EVENT_CONNECTION_STATE,
        addr);
    if (!msg)
        return;

    msg->data.valueint1 = state;
    lea_client_send_message(msg);
}

void lea_client_on_storage_changed(void* value, uint32_t size)
{
    lea_client_msg_t* msg = lea_client_msg_new_ext(STACK_EVENT_STORAGE, NULL,
        value, size);
    if (!msg)
        return;

    lea_client_send_message(msg);
}

void lea_client_on_pac_event(bt_address_t* addr, lea_client_capability_t* cap)
{
    lea_client_device_t* device;

    device = find_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&device->device_lock);
    memcpy(&device->pac[device->pac_number], cap, sizeof(lea_client_capability_t));
    device->pac_number++;
    pthread_mutex_unlock(&device->device_lock);
}

void lea_client_on_ascs_event(bt_address_t* addr, uint8_t ase_state, bool is_source, uint8_t ase_id)
{
    lea_client_device_t* device;
    int index;
    bool found = false;

    device = find_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&device->device_lock);
    for (index = 0; index < device->ase_number; index++) {
        if (device->ase[index].ase_id == ase_id) {
            device->ase[index].ase_state = ase_state;
            found = true;
            break;
        }
    }

    if (!found) {
        device->ase[device->ase_number].is_source = is_source;
        device->ase[device->ase_number].ase_id = ase_id;
        device->ase_number++;
    }

    pthread_mutex_unlock(&device->device_lock);
}

void lea_client_on_ascs_completed(bt_address_t* addr, uint32_t stream_id, uint8_t operation, uint8_t status)
{
    lea_client_group_t* group;
    lea_client_event_t event;
    lea_client_msg_t* msg;

    group = find_group_by_addr(addr);
    if (!group) {
        BT_LOGE("%s, group not exist", __func__);
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

    msg = lea_client_msg_new(event, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    msg->data.valueint2 = status;
    msg->data.valueint3 = group->group_id;
    lea_client_send_message(msg);
}

void lea_client_on_audio_localtion_event(bt_address_t* addr, bool is_source, uint32_t allcation)
{
    lea_client_device_t* device;

    device = find_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&device->device_lock);
    if (is_source) {
        device->source_allocation = allcation;
    } else {
        device->sink_allocation = allcation;
    }
    pthread_mutex_unlock(&device->device_lock);
}

void lea_client_on_available_audio_contexts_event(bt_address_t* addr, uint32_t sink_ctxs, uint32_t source_ctxs)
{
    lea_client_device_t* device;

    device = find_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, device not exist", __func__);
        return;
    }

    pthread_mutex_lock(&device->device_lock);
    device->source_avaliable_ctx = source_ctxs;
    device->sink_avaliable_ctx = sink_ctxs;
    pthread_mutex_unlock(&device->device_lock);
}

void lea_client_on_supported_audio_contexts_event(bt_address_t* addr, uint32_t sink_ctxs, uint32_t source_ctxs)
{
    lea_client_device_t* device;

    device = find_device_by_addr(addr);
    if (!device) {
        BT_LOGE("%s, devicenot exist", __func__);
        return;
    }

    pthread_mutex_lock(&device->device_lock);
    device->source_supported_ctx = source_ctxs;
    device->sink_supported_ctx = sink_ctxs;
    pthread_mutex_unlock(&device->device_lock);
}

void lea_client_on_stream_added(bt_address_t* addr, uint32_t stream_id)
{
    lea_client_msg_t* msg = lea_client_msg_new(STACK_EVENT_STREAM_ADDED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_removed(bt_address_t* addr, uint32_t stream_id)
{
    lea_client_msg_t* msg = lea_client_msg_new(STACK_EVENT_STREAM_REMOVED, addr);
    if (!msg)
        return;

    msg->data.valueint1 = stream_id;
    lea_client_send_message(msg);
}

void lea_client_on_stream_started(lea_audio_stream_t* audio)
{
    lea_audio_stream_t* stream;
    lea_client_msg_t* msg;

    stream = lea_client_find_stream(audio->stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, audio->stream_id);
        return;
    }

    if (!stream->is_source) {
        lea_audio_source_set_callback(&lea_source_callbacks);
    } else {
        lea_audio_sink_set_callback(&lea_sink_callbacks);
    }

    msg = lea_client_msg_new_ext(STACK_EVENT_STREAM_STARTED,
        &stream->addr, audio, sizeof(lea_audio_stream_t));
    if (!msg)
        return;

    lea_client_send_message(msg);
}

void lea_client_on_stream_stopped(uint32_t stream_id)
{
    lea_audio_stream_t* stream;
    lea_client_msg_t* msg;

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
    lea_audio_stream_t* stream;
    lea_client_msg_t* msg;

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
    lea_audio_stream_t* stream;
    lea_client_msg_t* msg;

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
    lea_audio_stream_t* stream;
    lea_client_msg_t* msg;

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
    uint16_t seq_number, uint8_t* sdu, uint16_t size)
{
    lea_audio_stream_t* stream;
    lea_recv_iso_data_t* packet;

    stream = lea_client_find_stream(stream_id);
    if (!stream) {
        BT_LOGE("%s, failed stream_id:0x%08x", __func__, stream_id);
        return;
    }

    packet = lea_audio_sink_packet_alloc(time_stamp, seq_number, sdu, size);
    if (!packet)
        return;

    // todo mix from many stream ?
    lea_audio_sink_packet_recv(packet);
}

void lea_client_on_csip_sirk_event(bt_address_t* addr, uint8_t type, uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_CS_SIRK, addr, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    msg->event_data.valueint8 = type;
    lea_csip_send_message(msg);
}

void lea_client_on_csip_size_event(bt_address_t* addr, uint8_t cs_size)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new(STACK_EVENT_CSIP_CS_SIZE, addr);
    if (!msg)
        return;

    msg->event_data.valueint8 = cs_size;
    lea_csip_send_message(msg);
}

void lea_client_on_csip_member_lock(bt_address_t* addr, uint8_t lock)
{
    lea_csip_msg_t* msg;
    uint8_t event;

    event = lock ? STACK_EVENT_CSIP_MEMBER_LOCKED : STACK_EVENT_CSIP_MEMBER_UNLOCKED;
    msg = lea_csip_msg_new(event, addr);
    if (!msg)
        return;

    msg->event_data.valueint8 = lock;
    lea_csip_send_message(msg);
}

void lea_client_on_csip_member_rank_event(bt_address_t* addr, uint8_t rank)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new(STACK_EVENT_CSIP_MEMBER_RANK, addr);
    if (!msg)
        return;

    msg->event_data.valueint8 = rank;
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_created(uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_CS_CREATED, NULL, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_size_updated(uint8_t* sirk, uint8_t cs_size)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_CS_SIZE_UPDATED, NULL, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    msg->event_data.valueint8 = cs_size;
    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);

    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_removed(uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_CS_DELETED, NULL, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_member_discovered(bt_address_t* addr, uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_MEMBER_DISCOVERED, addr, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_member_added(bt_address_t* addr, uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_MEMBER_ADD, addr, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_member_removed(bt_address_t* addr, uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_MEMBER_REMOVED, addr, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    lea_csip_send_message(msg);
}

void lea_client_on_csip_discovery_terminated(uint8_t* sirk)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_MEMBER_DISCOVERY_TERMINATED, NULL, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_lock_changed(uint8_t* sirk, bool locked, lea_csip_lock_status result)
{
    lea_csip_msg_t* msg;
    lea_csip_event_t event;

    if (locked) {
        event = STACK_EVENT_CSIP_CS_LOCKED;
    } else {
        event = STACK_EVENT_CSIP_CS_UNLOCKED;
    }

    msg = lea_csip_msg_new_ext(event, NULL, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    msg->event_data.valueint8 = result;
    lea_csip_send_message(msg);
}

void lea_client_on_csip_set_ordered_access(uint8_t* sirk, lea_csip_lock_status result)
{
    lea_csip_msg_t* msg;

    msg = lea_csip_msg_new_ext(STACK_EVENT_CSIP_CS_ORDERED_ACCESS, NULL, LEA_CLIENT_SRIK_SIZE);
    if (!msg)
        return;

    memcpy(msg->event_data.dataarry, sirk, LEA_CLIENT_SRIK_SIZE);
    msg->event_data.valueint8 = result;
    lea_csip_send_message(msg);
}

static const profile_service_t lea_client_service = {
    .auto_start = true,
    .name = PROFILE_LEA_CLIENT_NAME,
    .id = PROFILE_LEAUDIO_CLIENT,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_client_init,
    .startup = lea_client_startup,
    .shutdown = lea_client_shutdown,
    .process_msg = lea_client_process_msg,
    .get_state = lea_client_get_state,
    .get_profile_interface = get_leac_profile_interface,
    .cleanup = lea_client_cleanup,
    .dump = lea_client_dump,
};

void register_lea_client_service(void)
{
    register_service(&lea_client_service);
}
