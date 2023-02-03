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

#include "adapter_internel.h"
#include "manager_service.h"
#include "service_loop.h"
#include "stack_manager.h"
#include "state_machine.h"
#include "storage.h"

#ifdef CONFIG_BLUETOOTH_HFP_HF
#include "hfp_hf_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_HFP_AG
#include "hfp_ag_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_SPP
#include "spp_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_PAN
#include "pan_service.h"
#endif
#define LOG_TAG "bt_service"
#include "utils/log.h"

typedef struct {
    uint16_t profile_id;
    uint16_t event_id;
    void *data;
} service_msg_t;

typedef struct {
    state_machine_t *sm;
    uint16_t event_id;
    void *data;
} state_maechine_msg_t;

void bt_profile_init(void)
{
#ifdef CONFIG_BLUETOOTH_HFP_HF
    register_hfp_hf_service();
#endif

#ifdef CONFIG_BLUETOOTH_HFP_AG
    register_hfp_ag_service();
#endif

#ifdef CONFIG_BLUETOOTH_SPP
    register_spp_service();
#endif

#ifdef CONFIG_BLUETOOTH_PAN
    register_pan_service();
#endif
}

void bt_service_event_dispatch(void *smsg)
{
    free(smsg);
}

void bt_service_state_machine_event_dispatch(void *smsg)
{
    state_maechine_msg_t *stm_msg = smsg;

    hsm_dispatch_event(stm_msg->sm, stm_msg->event_id, stm_msg->data);
    free(smsg);
}

void send_to_profile_service(uint16_t profile_id, uint16_t event_id, void *data)
{
    service_msg_t *svc_msg = malloc(sizeof(service_msg_t));
    if (!svc_msg) {
        BT_LOGE("error, svc_msg malloc failed");
        return;
    }

    svc_msg->profile_id = profile_id;
    svc_msg->event_id = event_id;
    svc_msg->data = data;
    do_in_service_loop(bt_service_event_dispatch, svc_msg);
}

void send_to_state_machine(state_machine_t *sm, uint16_t event_id, void *data)
{
    state_maechine_msg_t *stm_msg = malloc(sizeof(state_maechine_msg_t));
    if (!stm_msg) {
        BT_LOGE("error, stm_msg malloc failed");
        return;
    }

    stm_msg->sm = sm;
    stm_msg->event_id = event_id;
    stm_msg->data = data;
    do_in_service_loop(bt_service_state_machine_event_dispatch, stm_msg);
}

int bt_service_init(void)
{
    utils_log_init();
    bt_storage_init();
    bt_profile_init();
    adapter_init();
    manager_init();
    stack_manager_init();

    BT_LOGD("%s done", __func__);
    return 0;
}

int bt_service_cleanup(void)
{
    stack_manager_cleanup();
    manager_cleanup();
    adapter_cleanup();
    bt_storage_cleanup();

    BT_LOGD("%s done", __func__);
    return 0;
}