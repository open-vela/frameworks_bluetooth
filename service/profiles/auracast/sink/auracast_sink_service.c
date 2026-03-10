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

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
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