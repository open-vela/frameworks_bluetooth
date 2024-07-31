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
#ifdef CONFIG_BLUETOOTH_GATT
#include "gattc_service.h"
#include "gatts_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_SPP
#include "spp_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_HID_DEVICE
#include "hid_device_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_PAN
#include "pan_service.h"
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
#include "lea_server_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCP
#include "lea_mcp_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
#include "lea_ccp_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICS
#include "lea_vmics_service.h"
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
#include "lea_client_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCS
#include "lea_mcs_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
#include "lea_tbs_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICP
#include "lea_vmicp_service.h"
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
#include "a2dp_sink_service.h"
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
#include "a2dp_source_service.h"
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
#include "avrcp_target_service.h"
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
#include "avrcp_control_service.h"
#endif

#define LOG_TAG "bt_service"
#include "utils/log.h"

#define MISC_PATH "/data/misc"
#define BT_FOLDER_PATH MISC_PATH "/" \
                                 "bt"

typedef struct {
    uint16_t profile_id;
    uint16_t event_id;
    void* data;
} service_msg_t;

typedef struct {
    state_machine_t* sm;
    uint16_t event_id;
    void* data;
} state_maechine_msg_t;

void bt_profile_init(void)
{
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    register_a2dp_sink_service();
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    register_a2dp_source_service();
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_TARGET
    register_avrcp_target_service();
#endif

#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    register_avrcp_control_service();
#endif

#ifdef CONFIG_BLUETOOTH_HFP_HF
    register_hfp_hf_service();
#endif

#ifdef CONFIG_BLUETOOTH_HFP_AG
    register_hfp_ag_service();
#endif

#ifdef CONFIG_BLUETOOTH_SPP
    register_spp_service();
#endif

#ifdef CONFIG_BLUETOOTH_HID_DEVICE
    register_hid_device_service();
#endif

#ifdef CONFIG_BLUETOOTH_PAN
    register_pan_service();
#endif

#ifdef CONFIG_BLUETOOTH_GATT
    register_gattc_service();
    register_gatts_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
    register_lea_server_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCP
    register_lea_mcp_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
    register_lea_ccp_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICS
    register_lea_vmics_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
    register_lea_client_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCS
    register_lea_mcs_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
    register_lea_tbs_service();
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICP
    register_lea_vmicp_service();
#endif
}

static int create_bt_folder(void)
{
    int ret = 0;

    if (mkdir(MISC_PATH, 0777) == -1 && errno != EEXIST) {
        ret = -1;
        syslog(LOG_ERR, MISC_PATH " folder create fail, errno: %d\n", errno);
        goto out;
    }

    if (mkdir(BT_FOLDER_PATH, 0777) == -1 && errno != EEXIST) {
        ret = -1;
        syslog(LOG_ERR, BT_FOLDER_PATH " folder create fail, errno: %d\n", errno);
        goto out;
    }

out:
    syslog(LOG_INFO, BT_FOLDER_PATH " folder create: %d\n", ret);
    return ret;
}

void bt_service_event_dispatch(void* smsg)
{
    free(smsg);
}

void bt_service_state_machine_event_dispatch(void* smsg)
{
    state_maechine_msg_t* stm_msg = smsg;

    hsm_dispatch_event(stm_msg->sm, stm_msg->event_id, stm_msg->data);
    free(smsg);
}

void send_to_profile_service(uint16_t profile_id, uint16_t event_id, void* data)
{
    service_msg_t* svc_msg = malloc(sizeof(service_msg_t));
    if (!svc_msg) {
        BT_LOGE("error, svc_msg malloc failed");
        return;
    }

    svc_msg->profile_id = profile_id;
    svc_msg->event_id = event_id;
    svc_msg->data = data;
    do_in_service_loop(bt_service_event_dispatch, svc_msg);
}

void send_to_state_machine(state_machine_t* sm, uint16_t event_id, void* data)
{
    state_maechine_msg_t* stm_msg = malloc(sizeof(state_maechine_msg_t));
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
    if (create_bt_folder() != 0)
        return -1;

    bt_log_server_init();
    bt_storage_init();
    bt_profile_init();
    adapter_init();
    manager_init();

    if (stack_manager_init() != BT_STATUS_SUCCESS)
        return -1;

    BT_LOGD("%s done", __func__);
    return 0;
}

int bt_service_cleanup(void)
{
    stack_manager_cleanup();
    manager_cleanup();
    adapter_cleanup();
    bt_storage_cleanup();
    bt_log_server_cleanup();

    BT_LOGD("%s done", __func__);
    return 0;
}
