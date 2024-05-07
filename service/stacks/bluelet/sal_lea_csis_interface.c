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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER

#include "bluetooth.h"
#include "bt_status.h"
#include "lea_audio_common.h"
#include "lea_server_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_csis_interface.h"

static void adpt_lea_csis_member_lock_cbk(uint32_t csis_id, BD_ADDR remote_addr, uint8_t lock);

const LEA_CSIS_CALLBACK_S adpt_lea_csip_server_callbacks = {
    .lea_csis_member_lock_cb = adpt_lea_csis_member_lock_cbk,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_csis_member_lock_cbk(uint32_t csis_id, BD_ADDR remote_addr,
    uint8_t lock)
{
    bt_address_t addr;

    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_server_on_csis_lock_state_changed(csis_id, &addr, lock);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bool adpt_req_csis_info_callback(SERVICE_LEA_CSIS_S* info)
{
    return lea_server_on_csis_info_request((lea_csis_infos_t*)info);
}

#endif /* __SAL_LEA_CSIS_INTERFACE_H__ */