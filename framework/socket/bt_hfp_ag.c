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
#define LOG_TAG "hfp_ag_api"

#include "bt_hfp_ag.h"
#include "bt_profile.h"
#include "hfp_ag_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

void *bt_hfp_ag_register_callbacks(bt_instance_t *ins, const hfp_ag_callbacks_t *callbacks)
{
    return NULL;
}

bool bt_hfp_ag_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
    return false;
}

bool bt_hfp_ag_is_connected(bt_instance_t *ins, bt_address_t *addr)
{
    return false;
}

bool bt_hfp_ag_is_audio_connected(bt_instance_t *ins, bt_address_t *addr)
{
    return false;
}

profile_connection_state_t bt_hfp_ag_get_connection_state(bt_instance_t *ins, bt_address_t *addr)
{
    return PROFILE_STATE_DISCONNECTED;
}

bt_status_t bt_hfp_ag_connect(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_hfp_ag_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_hfp_ag_connect_audio(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_hfp_ag_disconnect_audio(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_hfp_ag_start_voice_recognition(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_hfp_ag_stop_voice_recognition(bt_instance_t *ins, bt_address_t *addr)
{
    return BT_STATUS_SUCCESS;
}
