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

static void adpt_lea_csis_member_lock_cbk(uint32_t csis_id, BD_ADDR csis_addr, uint8_t lock);

static const char *default_sirk = "13579a24680b";

const LEA_CSIS_CALLBACK_S adpt_lea_csip_server_callbacks = {
    .lea_csis_member_lock_cb = adpt_lea_csis_member_lock_cbk,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_csis_recycle_func(uint32_t number, void *csis_s)
{
    free(csis_s);
}

static void adpt_lea_csis_member_lock_cbk(uint32_t csis_id, BD_ADDR csis_addr,
                                          uint8_t lock)
{
    char *lock_s[] = { "NA", "Unlocked", "Locked" };
    BT_LOGD("%s, [CSIS 0x%08x][%s]", __func__, csis_id, lock_s[lock]);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bool adpt_req_csis_info_callback(SERVICE_LEA_CSIS_S *info)
{
    char value[BT_COMMON_KEY_SIZE] = { 0 };

    BT_LOGD("%s", __func__);
    info->csis_number = 1;
    info->csis_info = malloc(sizeof(SERVICE_LEA_CSIS_INFO_S));
    info->csis_info[0].csis_id = 0x47;
    info->csis_info[0].set_size = property_get_int32("persist.bluetooth.csis.set_size", 1);
    info->csis_info[0].sirk_type = LEA_SIRK_TYPE_ENCRYPTED;
    info->csis_info[0].rank = property_get_int32("persist.bluetooth.csis.rank", 1);

    property_get("persist.bluetooth.csis.set_sirk", value, default_sirk);
    for (int i = 0; i < BT_COMMON_KEY_SIZE; i++) {
        info->csis_info[0].sirk[i] = strtol(&value[i], NULL, 16);
    }

    info->recycle_func = adpt_lea_csis_recycle_func;
    return TRUE;
}
#endif /* __SAL_LEA_CSIS_INTERFACE_H__ */