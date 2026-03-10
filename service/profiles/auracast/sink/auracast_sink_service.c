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

static void service_startup(const auracast_sink_msg_t* msg)
{
    auracast_sink_service_t* service = &g_auracast_sink_service;
    profile_on_startup_t cb = (profile_on_startup_t)msg->context;

    cb(PROFILE_AURACAST_SINK, true);
    return;
}

static void service_shutdown(const auracast_sink_msg_t* msg)
{
    auracast_sink_service_t* service = &g_auracast_sink_service;
    profile_on_shutdown_t cb = (profile_on_shutdown_t)msg->context;

    cb(PROFILE_AURACAST_SINK, true);
    return;
}

static void auracast_sink_process_message(void* data)
{
    auracast_sink_msg_t* msg = (auracast_sink_msg_t*)data;

    auracast_sink_msg_destory(msg);
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
    return NULL;
}

static bool auracast_sink_unregister_callbacks(void** remote, void* cookie)
{
    return true;
}

static bt_status_t auracast_sink_create_sync(const bt_le_address_t* addr, uint8_t sid,
    uint32_t bitfield, const uint8_t* broadcast_code)
{
    return BT_STATUS_SUCCESS;
}

static bt_status_t auracast_sink_terminate_sync(const bt_le_address_t* addr, uint8_t sid)
{
    return BT_STATUS_SUCCESS;
}

static bt_status_t auracast_sink_dump(void)
{
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
