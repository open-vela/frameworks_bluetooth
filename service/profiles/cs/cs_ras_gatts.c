/****************************************************************************
 *
 *   Copyright (C) 2025 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include "bt_gatts.h"
#include "bt_profile.h"
#include "bt_status.h"
#include "cs_ras_gatts.h"
#include "gatts_service.h"
#include "cs_ras_util.h"
#include "utils/log.h"
#include "service_manager.h"

typedef struct {
    gatts_handle_t ras_gatts_handle;
    bt_address_t addr;
    const gatts_interface_t* ras_gatts_interface;
    const ras_gatts_callbacks_t* cb;
} ras_gatt_info_t;

static ras_gatt_info_t* gatts_info = NULL;

static uint16_t ras_real_time_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset);

static uint16_t ras_on_demand_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset);

static uint16_t ras_control_point_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset);

static uint16_t ras_data_ready_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset);
static uint16_t ras_over_write_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset);
static uint16_t ras_control_point_write_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset);
static uint16_t ras_feature_read_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle);
static void cs_ras_gatts_connected_cb(gatts_handle_t srv_handle, bt_address_t* addr);
static void cs_ras_gatts_disconnected_cb(gatts_handle_t srv_handle, bt_address_t* addr);
static void cs_ras_notify_complete_cb(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle);
static void cs_ras_mtu_change_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t mtu);

static gatt_attr_db_t ras_attr_db[] = {
    GATT_H_PRIMARY_SERVICE(BT_UUID_DECLARE_16(BT_UUID_RANGING_VAL), RAS_RANGING_SERVICE_ATTR_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANG_FEAT_VAL),
                     GATT_PROP_READ, GATT_PERM_READ, ras_feature_read_cb, NULL, RAS_RANGING_FEATURE_ATTR_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANG_RTT_DT_VAL),
                    GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ,
                    NULL, NULL, RAS_RANGING_REAL_TIME_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, ras_real_time_ccc_changed_cb, RAS_RANGING_REAL_TIME_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANG_ON_DEM_DT_VAL),
                     GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ, NULL, NULL, RAS_RANGING_ON_DEMAND_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, ras_on_demand_ccc_changed_cb, RAS_RANGING_ON_DEMAND_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANG_RAS_CTR_POINT_VAL),
                     GATT_PROP_NOTIFY | GATT_PROP_INDICATE | GATT_PROP_WRITE_NR, GATT_PERM_READ | GATT_PERM_WRITE, NULL,
                     ras_control_point_write_cb, RAS_RANGING_CONTROL_POINT_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, ras_control_point_ccc_changed_cb, RAS_RANGING_CONTROL_POINT_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANG_DT_RD_VAL),
                     GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ,
                     NULL, NULL, RAS_RANGING_DATA_READY_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, ras_data_ready_ccc_changed_cb, RAS_RANGING_DATA_READY_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANG_DT_OV_WR_VAL),
                    GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ,
                    NULL, NULL, RAS_RANGING_DATA_OVER_WRITE_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, ras_over_write_ccc_changed_cb, RAS_RANGING_DATA_OVER_WRITE_CCC_ID),
};

static gatt_srv_db_t ras_service_db = {
    .attr_db = ras_attr_db,
    .attr_num = sizeof(ras_attr_db) / sizeof(gatt_attr_db_t),
};

static uint16_t ras_real_time_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_RTT_DATA_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t ras_on_demand_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_ON_DEMAND_DATA_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t ras_control_point_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_CTR_PT_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t ras_data_ready_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_DATA_READY_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t ras_over_write_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_OVER_WRITE_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

uint16_t ras_control_point_write_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
              const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
         BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->pt_write_cb) {
        gatts_info->cb->pt_write_cb(addr, value, length);
    }

    return length;
}

static uint16_t ras_feature_read_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info && gatts_info->cb->cfg_cb) {
        gatts_info->cb->feature_read_cb(addr, req_handle);
    }

    return 0;
}

bt_status_t ras_send_feature_read_rsp(bt_address_t* addr, uint32_t feature)
{
    if (gatts_info->ras_gatts_handle == NULL) {
        BT_LOGE("Invalid gatts handle.");
        return BT_STATUS_FAIL;
    }

    uint8_t feature_val[4];
    uint8_t *p = feature_val;
    CS_UINT32_TO_BYTE_STREAM(p, feature);
    bt_status_t status = bt_gatts_response(gatts_info->ras_gatts_handle, addr, RAS_RANGING_FEATURE_ATTR_ID,
                                 feature_val, sizeof(feature_val));

    return status;
}

static gatts_callbacks_t ras_gatts_callbacks = {
    .size = sizeof(ras_gatts_callbacks),
    .on_connected = cs_ras_gatts_connected_cb,
    .on_disconnected = cs_ras_gatts_disconnected_cb,
    .on_attr_table_added = NULL,
    .on_attr_table_removed = NULL,
    .on_notify_complete = cs_ras_notify_complete_cb,
    .on_mtu_changed = cs_ras_mtu_change_cb,
    .on_phy_read = NULL,
    .on_phy_updated = NULL,
    .on_conn_param_changed = NULL,
};

static void ras_gatts_register(void)
{
    const gatts_interface_t* interface = gatts_info->ras_gatts_interface;

    BT_LOGD("%s", __func__);

    if (gatts_info->ras_gatts_handle) {
        BT_LOGW("The RAS gatt Server has been register.");
        return;
    }

    interface->register_service(NULL, &gatts_info->ras_gatts_handle, &ras_gatts_callbacks);
    if (!gatts_info->ras_gatts_handle) {
        BT_LOGE("%s, failed to register service", __func__);
        return;
    }

    if (interface->add_attr_table(gatts_info->ras_gatts_handle, &ras_service_db)
        != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, failed to add attribute", __func__);
        return;
    }

    BT_LOGD("%s, wait for service registered", __func__);
}

static void gatts_unregister(void)
{
    const gatts_interface_t* interface = gatts_info->ras_gatts_interface;

    BT_LOGD("%s", __func__);

    if (gatts_info->ras_gatts_handle) {
        interface->unregister_service(gatts_info->ras_gatts_handle);
        gatts_info->ras_gatts_handle = NULL;
    }
}

void bt_cs_ras_gatts_init(const ras_gatts_callbacks_t* callback)
{
    BT_LOGD("%s", __func__);

    if (!gatts_info)
        gatts_info = zalloc(sizeof(ras_gatt_info_t));

    gatts_info->ras_gatts_interface = (gatts_interface_t*)service_manager_get_profile(PROFILE_GATTS);

    if (gatts_info->ras_gatts_interface == NULL) {
        BT_LOGE("ras gatts interface get fail.");
        return;
    }

    gatts_info->cb = callback;

    ras_gatts_register();
}

void ras_gatts_deinit(void)
{
    BT_LOGD("%s", __func__);

    if (!gatts_info)
        return;

    gatts_unregister();
    free(gatts_info);
    gatts_info = NULL;
}

void cs_ras_gatts_connected_cb(gatts_handle_t srv_handle, bt_address_t* addr)
{
    cs_msg_t* msg = cs_msg_new(CONNECTED_EVT, addr);
    bt_sal_cs_event_callback(msg);

    if (gatts_info && gatts_info->cb->conn_cb) {
        gatts_info->cb->conn_cb(addr);
    }

    return;
}

void cs_ras_gatts_disconnected_cb(gatts_handle_t srv_handle, bt_address_t* addr)
{
    cs_msg_t* msg = cs_msg_new(DISCONNECTED_EVT, addr);
    bt_sal_cs_event_callback(msg);

    if (gatts_info && gatts_info->cb->disconn_cb) {
        gatts_info->cb->disconn_cb(addr);
    }

    return;
}

static void cs_ras_notify_complete_cb(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return;
    }

    ras_attr_notify_t attr_ntf;
    switch (attr_handle)
    {
    case RAS_RANGING_REAL_TIME_CCC_ID:
        attr_ntf = RAS_REAL_TIME_CHAR_SEND;
        break;
    case RAS_RANGING_ON_DEMAND_CCC_ID:
        attr_ntf = RAS_ON_DEMAND_CHAR_SEND;
        break;
    case RAS_RANGING_CONTROL_POINT_CCC_ID:
        attr_ntf = RAS_CONTROL_POINT_CHAR_SEND;
        break;
    case RAS_RANGING_DATA_READY_CCC_ID:
        attr_ntf = RAS_DATA_READY_CHAR_SEND;
        break;
    case RAS_RANGING_DATA_OVER_WRITE_CCC_ID:
        attr_ntf = RAS_OVER_WRITE_CHAR_SEND;
        break;
    default:
        BT_LOGE("Invalid attr_handle:%d", attr_handle);
        return;
    }

    if (gatts_info && gatts_info->ras_gatts_handle && gatts_info->cb->notify_cb) {
        gatts_info->cb->notify_cb(addr, status, attr_ntf);
    }

    return;
}

static void cs_ras_mtu_change_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t mtu)
{
    if (gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return;
    }

    if (gatts_info && gatts_info->cb->mtu_updated_cb) {
        gatts_info->cb->mtu_updated_cb(addr, mtu);
    }

    return;
}

bt_status_t ras_gatts_data_send_notify(ras_attr_notify_t attr, bt_address_t* addr,
                            uint8_t* value, uint16_t len, bool is_notify)
{
    if (!gatts_info || !value || !addr) {
        BT_LOGE("Invalid params");
        return BT_STATUS_PARM_INVALID;
    }

    uint16_t attr_handle;
    switch(attr) {
    case RAS_REAL_TIME_CHAR_SEND:
        attr_handle = RAS_RANGING_REAL_TIME_ATTR_ID;
        break;
    case RAS_ON_DEMAND_CHAR_SEND:
        attr_handle = RAS_RANGING_ON_DEMAND_ATTR_ID;
        break;
    case RAS_DATA_READY_CHAR_SEND:
        attr_handle = RAS_RANGING_DATA_READY_ATTR_ID;
        break;
    case RAS_CONTROL_POINT_CHAR_SEND:
        attr_handle = RAS_RANGING_CONTROL_POINT_ATTR_ID;
        break;
    case RAS_OVER_WRITE_CHAR_SEND:
        attr_handle = RAS_RANGING_DATA_OVER_WRITE_ATTR_ID;
        break;
    default:
        BT_LOGE("Invalid attr_ntf type:%d", attr);
        return BT_STATUS_FAIL;
    }

    bt_status_t status = (is_notify ? (gatts_info->ras_gatts_interface->notify(gatts_info->ras_gatts_handle, addr, attr_handle, value, len)) :\
                         gatts_info->ras_gatts_interface->indicate(gatts_info->ras_gatts_handle, addr, attr_handle, value, len));
    return status;
}

