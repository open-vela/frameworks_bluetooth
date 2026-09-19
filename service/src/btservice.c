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
#include "cs_service.h"
#include "manager_service.h"
#include "service_loop.h"
#include "stack_manager.h"
#include "state_machine.h"
#include "storage.h"

#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

/* TEMP-DIAG: probe inode tree health by opening /dev/urandom (the exact
 * path that hardfaults when the inode list is corrupted). */
static void probe_inode(const char *tag)
{
  int fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  syslog(LOG_INFO, "[probe] %s: open=%d", tag, fd);
  if (fd >= 0)
    {
      close(fd);
    }
}

#ifdef CONFIG_BLUETOOTH_HFP_HF
#include "hfp_hf_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_HFP_AG
#include "hfp_ag_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
#include "gattc_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_GATT_SERVER
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

#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
    register_gattc_service();
#endif
#ifdef CONFIG_BLUETOOTH_GATT_SERVER
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
#ifdef CONFIG_BLUETOOTH_LE_CS
    register_cs_service();
#endif /* CONFIG_BLUETOOTH_LE_CS */
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

#ifdef CONFIG_BLUETOOTH_AUTO_ENABLE
/* Nothing in this tree calls bt_adapter_enable() on its own: bluetoothd only
 * builds the stack, and the only client that wants the radio (ai_agent) just
 * watches for a bt-pan address to appear. So a board that boots straight into
 * the product image came up with Bluetooth off and stayed there until someone
 * typed "bttool enable" on the console. Turn it on here instead.
 *
 * Deferred rather than called inline: adapter_enable() posts SYS_TURN_ON to
 * the adapter state machine, and the whole turn-on path (HCI reset, LCPU
 * command round-trips, profile bring-up) is the service loop's work. The loop
 * is not running yet at bt_service_init() time - main() starts it right
 * afterwards - so this arms a timer and lets the loop run it. uv_timer_start()
 * accepts that and fires as soon as the loop comes up and the delay has
 * elapsed.
 */
static void bt_auto_enable_timeout(service_timer_t* timer, void* userdata)
{
    bt_status_t status;

    (void)userdata;
    service_loop_cancel_timer(timer);

    status = adapter_enable(SYS_SET_BT_ALL);
    syslog(LOG_INFO, "bluetoothd auto-enable: adapter_enable=%d\n",
        (int)status);
}

static void bt_auto_enable_arm(void)
{
    if (!service_loop_timer_no_repeating(CONFIG_BLUETOOTH_AUTO_ENABLE_DELAY_MS,
            bt_auto_enable_timeout, NULL)) {
        syslog(LOG_ERR, "bluetoothd auto-enable: no timer\n");
    }
}
#endif

int bt_service_init(void)
{
    probe_inode("btsvc-entry");

    if (create_bt_folder() != 0)
        return -1;

    probe_inode("btsvc-folder");

#ifdef CONFIG_BLUETOOTH_LOG
    bt_log_server_init();
#endif
    bt_storage_init();
    probe_inode("btsvc-storage");

    bt_profile_init();
    probe_inode("btsvc-profile");

    adapter_init();
    manager_init();
    probe_inode("btsvc-adapter-mgr");

    if (stack_manager_init() != BT_STATUS_SUCCESS)
        return -1;

    probe_inode("btsvc-stackmgr");

#ifdef CONFIG_BLUETOOTH_AUTO_ENABLE
    bt_auto_enable_arm();
#endif

    BT_LOGD("%s done", __func__);
    return 0;
}

int bt_service_cleanup(void)
{
    stack_manager_cleanup();
    manager_cleanup();
    adapter_cleanup();
    bt_storage_cleanup();

#ifdef CONFIG_BLUETOOTH_LOG
    bt_log_server_cleanup();
#endif

    BT_LOGD("%s done", __func__);
    return 0;
}
