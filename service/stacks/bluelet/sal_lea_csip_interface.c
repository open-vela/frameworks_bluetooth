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

#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT

#include "stack_adapter_gap.h"
#include "stack_adapter_lea_gaf.h"

#include "bluetooth.h"
#include "bt_status.h"
#include "lea_audio_common.h"
#include "lea_client_service.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_lea_csip_interface.h"

static void adpt_lea_csip_sirk_callback(BD_ADDR remote_addr, uint8_t type, LEA_CSIP_SIRK sirk);
static void adpt_lea_csip_cs_size_callback(BD_ADDR remote_addr, uint8_t cs_size);
static void adpt_lea_csip_member_lock_callback(BD_ADDR remote_addr, uint8_t lock);
static void adpt_lea_csip_member_rank_callback(BD_ADDR remote_addr, uint8_t rank);
static void adpt_lea_csip_set_created_callback(LEA_CSIP_SIRK sirk);
static void adpt_lea_csip_set_size_updated_callback(LEA_CSIP_SIRK sirk, uint8_t cs_size);
static void adpt_lea_csip_set_removed_callback(LEA_CSIP_SIRK sirk);
static void adpt_lea_csip_set_member_discovered_callback(LEA_CSIP_SIRK sirk, BD_ADDR remote_addr);
static void adpt_lea_csip_set_member_added_callback(LEA_CSIP_SIRK sirk, BD_ADDR remote_addr);
static void adpt_lea_csip_set_member_removed_callback(LEA_CSIP_SIRK sirk, BD_ADDR remote_addr);
static void adpt_lea_csip_discovery_terminated_callback(LEA_CSIP_SIRK sirk);
static void adpt_lea_csip_lock_request_result_callback(LEA_CSIP_SIRK sirk, SERVICE_LEA_CSIS_LOCK_RESULT result);
static void adpt_lea_csip_lock_release_result_callback(LEA_CSIP_SIRK sirk, SERVICE_LEA_CSIS_LOCK_RESULT result);
static void adpt_lea_csip_ordered_access_result_callback(LEA_CSIP_SIRK sirk, SERVICE_LEA_CSIS_LOCK_RESULT result);

const LEA_CSIC_CALLBACK_S adpt_lea_csip_client_callbacks = {
    .lea_csic_sirk_cb = adpt_lea_csip_sirk_callback,
    .lea_csic_cs_size_cb = adpt_lea_csip_cs_size_callback,
    .lea_csic_member_lock_cb = adpt_lea_csip_member_lock_callback,
    .lea_csic_member_rank_cb = adpt_lea_csip_member_rank_callback,

    .lea_csic_set_created_cb = adpt_lea_csip_set_created_callback,
    .lea_csic_set_size_updated_cb = adpt_lea_csip_set_size_updated_callback,
    .lea_csic_set_removed_cb = adpt_lea_csip_set_removed_callback,
    .lea_csic_set_member_discovered_cb = adpt_lea_csip_set_member_discovered_callback,
    .lea_csic_set_member_added_cb = adpt_lea_csip_set_member_added_callback,
    .lea_csic_set_member_removed_cb = adpt_lea_csip_set_member_removed_callback,

    .lea_csic_discovery_terminated_cb = adpt_lea_csip_discovery_terminated_callback,
    .lea_csic_lock_request_result_cb = adpt_lea_csip_lock_request_result_callback,
    .lea_csic_lock_release_result_cb = adpt_lea_csip_lock_release_result_callback,
    .lea_csic_ordered_access_result_cb = adpt_lea_csip_ordered_access_result_callback,
};

/****************************************************************************
 * Private function
 ****************************************************************************/

static void adpt_lea_csip_sirk_callback(BD_ADDR remote_addr, uint8_t type, LEA_CSIP_SIRK sirk)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_sirk_event(&addr, type, sirk);
}

static void adpt_lea_csip_cs_size_callback(BD_ADDR remote_addr, uint8_t cs_size)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_size_event(&addr, cs_size);
}

static void adpt_lea_csip_member_lock_callback(BD_ADDR remote_addr, uint8_t lock)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_member_lock(&addr, lock);
}

static void adpt_lea_csip_member_rank_callback(BD_ADDR remote_addr, uint8_t rank)
{
    bt_address_t addr;

    BT_LOGD("%s, rank:%d", __func__, rank);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_member_rank_event(&addr, rank);
}

static void adpt_lea_csip_set_created_callback(LEA_CSIP_SIRK sirk)
{
    BT_LOGD("%s", __func__);
    lea_client_on_csip_set_created(sirk);
}

static void adpt_lea_csip_set_size_updated_callback(LEA_CSIP_SIRK sirk, uint8_t cs_size)
{
    BT_LOGD("%s, cs_size:%d", __func__, cs_size);
    lea_client_on_csip_set_size_updated(sirk, cs_size);
}

static void adpt_lea_csip_set_removed_callback(LEA_CSIP_SIRK sirk)
{
    BT_LOGD("%s", __func__);
    lea_client_on_csip_set_removed(sirk);
}

static void adpt_lea_csip_set_member_discovered_callback(LEA_CSIP_SIRK sirk, BD_ADDR remote_addr)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_set_member_discovered(&addr, sirk);
}

static void adpt_lea_csip_set_member_added_callback(LEA_CSIP_SIRK sirk, BD_ADDR remote_addr)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_set_member_added(&addr, sirk);
}

static void adpt_lea_csip_set_member_removed_callback(LEA_CSIP_SIRK sirk, BD_ADDR remote_addr)
{
    bt_address_t addr;

    BT_LOGD("%s", __func__);
    memcpy(addr.addr, remote_addr, sizeof(BD_ADDR));
    lea_client_on_csip_set_member_removed(&addr, sirk);
}

static void adpt_lea_csip_discovery_terminated_callback(LEA_CSIP_SIRK sirk)
{
    BT_LOGD("%s", __func__);
    lea_client_on_csip_discovery_terminated(sirk);
}

static void adpt_lea_csip_lock_request_result_callback(LEA_CSIP_SIRK sirk, SERVICE_LEA_CSIS_LOCK_RESULT result)
{
    BT_LOGD("%s, result:%d", __func__, result);
    lea_client_on_csip_set_lock_changed(sirk, true, result);
}

static void adpt_lea_csip_lock_release_result_callback(LEA_CSIP_SIRK sirk, SERVICE_LEA_CSIS_LOCK_RESULT result)
{
    BT_LOGD("%s, result:%d", __func__, result);
    lea_client_on_csip_set_lock_changed(sirk, false, result);
}

static void adpt_lea_csip_ordered_access_result_callback(LEA_CSIP_SIRK sirk, SERVICE_LEA_CSIS_LOCK_RESULT result)
{
    BT_LOGD("%s", __func__);
    lea_client_on_csip_set_ordered_access(sirk, result);
}

/****************************************************************************
 * Public function
 ****************************************************************************/

bt_status_t bt_sal_lea_csip_read_sirk(bt_address_t* addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_csic_read_sirk(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_read_cs_size(bt_address_t* addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_csic_read_cs_size(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_read_member_lock(bt_address_t* addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_csic_read_member_lock(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_read_member_rank(bt_address_t* addr)
{
    BD_ADDR bd_addr;

    memcpy(bd_addr, addr, sizeof(BD_ADDR));
    SAL_CHECK_RET(stack_adapter_lea_csic_read_member_rank(bd_addr), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_coordinated_set_discovery_member_start(uint8_t* sirk)
{
    SAL_CHECK_RET(stack_adapter_lea_csic_set_member_discovery(sirk), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_coordinated_set_discovery_member_stop(uint8_t* sirk)
{
    SAL_CHECK_RET(stack_adapter_lea_csic_set_member_discovery_cancel(sirk), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_coordinated_set_lock_request(uint8_t* sirk)
{
    SAL_CHECK_RET(stack_adapter_lea_csic_lock_request(sirk), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_coordinated_set_lock_release(uint8_t* sirk)
{
    SAL_CHECK_RET(stack_adapter_lea_csic_lock_release(sirk), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_lea_csip_coordinated_set_ordered_access(uint8_t* sirk)
{
    SAL_CHECK_RET(stack_adapter_lea_csic_ordered_access(sirk), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif /* __SAL_LEA_CSIP_INTERFACE_H__ */