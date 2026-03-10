/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#include "cs_ras_gatts.h"
#include "bt_gatts.h"
#include "bt_profile.h"
#include "bt_status.h"
#include "bt_utils.h"
#include "cs_msg.h"
#include "cs_ras_util.h"
#include "cs_service.h"
#include "gatts_service.h"
#include "service_manager.h"
#include "utils/log.h"

typedef struct {
    gatts_handle_t ras_gatts_handle;
    bt_address_t addr;
    const gatts_interface_t* ras_gatts_interface;
    const ras_gatts_callbacks_t* cb;
} ras_gatt_info_t;

static ras_gatt_info_t* gatts_info = NULL;

static uint16_t cs_ras_real_time_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);

static uint16_t cs_ras_on_demand_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);

static uint16_t cs_ras_control_point_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);

static uint16_t cs_ras_data_ready_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);
static uint16_t cs_ras_over_write_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);
static uint16_t cs_ras_control_point_write_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset);
static uint16_t cs_ras_feature_read_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle);
static void cs_ras_gatts_connected_cb(gatts_handle_t srv_handle, bt_address_t* addr);
static void cs_ras_gatts_disconnected_cb(gatts_handle_t srv_handle, bt_address_t* addr);
static void cs_ras_notify_complete_cb(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle);
static void cs_ras_mtu_change_cb(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t mtu);
static void cs_ras_gatts_unregister(void);

static const gatt_attr_db_t ras_attr_db[] = {
    GATT_H_PRIMARY_SERVICE(BT_UUID_DECLARE_16(BT_UUID_RANGING_VAL), RAS_RANGING_SERVICE_ATTR_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANGE_FEAT_VAL),
        GATT_PROP_READ, GATT_PERM_READ, cs_ras_feature_read_cb, NULL, RAS_RANGING_FEATURE_ATTR_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANGE_RTT_DT_VAL),
        GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ,
        NULL, NULL, RAS_RANGING_REAL_TIME_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, cs_ras_real_time_ccc_changed_cb, RAS_RANGING_REAL_TIME_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANGE_ON_DEM_DT_VAL),
        GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ, NULL, NULL, RAS_RANGING_ON_DEMAND_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, cs_ras_on_demand_ccc_changed_cb, RAS_RANGING_ON_DEMAND_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANGE_RAS_CTR_POINT_VAL),
        GATT_PROP_NOTIFY | GATT_PROP_INDICATE | GATT_PROP_WRITE_NR, GATT_PERM_READ | GATT_PERM_WRITE, NULL,
        cs_ras_control_point_write_cb, RAS_RANGING_CONTROL_POINT_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, cs_ras_control_point_ccc_changed_cb, RAS_RANGING_CONTROL_POINT_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANGE_DT_RD_VAL),
        GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ,
        NULL, NULL, RAS_RANGING_DATA_READY_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, cs_ras_data_ready_ccc_changed_cb, RAS_RANGING_DATA_READY_CCC_ID),
    GATT_H_CHARACTERISTIC_USER_RSP(BT_UUID_DECLARE_16(BT_UUID_RANGE_DT_OV_WR_VAL),
        GATT_PROP_NOTIFY | GATT_PROP_INDICATE, GATT_PERM_READ,
        NULL, NULL, RAS_RANGING_DATA_OVER_WRITE_ATTR_ID),
    GATT_H_CCCD(GATT_PERM_READ | GATT_PERM_WRITE, cs_ras_over_write_ccc_changed_cb, RAS_RANGING_DATA_OVER_WRITE_CCC_ID),
};

static const gatt_srv_db_t ras_service_db = {
    .attr_db = (gatt_attr_db_t*)ras_attr_db,
    .attr_num = ARRAY_SIZE(ras_attr_db),
};

static uint16_t cs_ras_real_time_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_RTT_DATA_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t cs_ras_on_demand_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_ON_DEMAND_DATA_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t cs_ras_control_point_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_CTR_PT_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t cs_ras_data_ready_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_DATA_READY_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

static uint16_t cs_ras_over_write_ccc_changed_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->cfg_cb) {
        gatts_info->cb->cfg_cb(addr, RAS_OVER_WRITE_CCC_CFG_CHANGE_EVT, value, length);
    }

    return length;
}

uint16_t cs_ras_control_point_write_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle,
    const uint8_t* value, uint16_t length, uint16_t offset)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->pt_write_cb) {
        gatts_info->cb->pt_write_cb(addr, value, length);
    }

    return length;
}

static uint16_t cs_ras_feature_read_cb(void* srv_handle, bt_address_t* addr, uint16_t attr_handle, uint32_t req_handle)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return 0;
    }

    if (gatts_info->cb->cfg_cb) {
        gatts_info->cb->feature_read_cb(addr, req_handle);
    }

    return 0;
}

bt_status_t ras_send_feature_read_rsp(bt_address_t* addr, uint32_t feature, uint32_t req_handle)
{
    const gatts_interface_t* interface = gatts_info->ras_gatts_interface;

    if (!gatts_info || gatts_info->ras_gatts_handle == NULL) {
        BT_LOGE("Invalid gatts handle.");
        return BT_STATUS_FAIL;
    }

    uint8_t feature_val[4];
    uint8_t* p = feature_val;
    UINT32_TO_STREAM(p, feature);

    return interface->response(gatts_info->ras_gatts_handle, addr, req_handle, feature_val, sizeof(feature_val));
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

    if (interface->add_attr_table(gatts_info->ras_gatts_handle, (gatt_srv_db_t*)&ras_service_db)
        != BT_STATUS_SUCCESS) {
        cs_ras_gatts_unregister();
        BT_LOGE("%s, failed to add attribute", __func__);
        return;
    }

    BT_LOGD("%s, wait for service registered", __func__);
}

static void cs_ras_gatts_unregister(void)
{
    if (!gatts_info) {
        BT_LOGE("%s, Invalid gatts_info(NULL).", __func__);
        return;
    }

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

    if (!callback) {
        BT_LOGE("Should set the callback when init the cs ras gatts.");
        return;
    }

    if (!gatts_info)
        gatts_info = zalloc(sizeof(ras_gatt_info_t));

    gatts_info->ras_gatts_interface = (gatts_interface_t*)service_manager_get_profile(PROFILE_GATTS);

    if (gatts_info->ras_gatts_interface == NULL) {
        BT_LOGE("ras gatts interface get fail.");
        free(gatts_info);
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

    cs_ras_gatts_unregister();
    free(gatts_info);
    gatts_info = NULL;
}

void cs_ras_gatts_connected_cb(gatts_handle_t srv_handle, bt_address_t* addr)
{
    cs_msg_t* msg = cs_msg_new(CONNECTED_EVT, addr);
    bt_sal_cs_event_callback(msg);

    if (gatts_info && gatts_info->cb && gatts_info->cb->conn_cb) {
        gatts_info->cb->conn_cb(addr);
    }
}

void cs_ras_gatts_disconnected_cb(gatts_handle_t srv_handle, bt_address_t* addr)
{
    cs_msg_t* msg = cs_msg_new(DISCONNECTED_EVT, addr);
    bt_sal_cs_event_callback(msg);

    if (gatts_info && gatts_info->cb->disconn_cb) {
        gatts_info->cb->disconn_cb(addr);
    }
}

static void cs_ras_notify_complete_cb(gatts_handle_t srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle)
{
    if (!gatts_info || gatts_info->ras_gatts_handle != srv_handle) {
        BT_LOGE("srv_handle(%p) not equal to gatts_handle(%p)", srv_handle, gatts_info->ras_gatts_handle);
        return;
    }

    ras_attr_notify_t attr_ntf;
    switch (attr_handle) {
    case RAS_RANGING_REAL_TIME_ATTR_ID:
        attr_ntf = RAS_REAL_TIME_CHAR_SEND;
        break;
    case RAS_RANGING_ON_DEMAND_ATTR_ID:
        attr_ntf = RAS_ON_DEMAND_CHAR_SEND;
        break;
    case RAS_RANGING_CONTROL_POINT_ATTR_ID:
        attr_ntf = RAS_CONTROL_POINT_CHAR_SEND;
        break;
    case RAS_RANGING_DATA_READY_ATTR_ID:
        attr_ntf = RAS_DATA_READY_CHAR_SEND;
        break;
    case RAS_RANGING_DATA_OVER_WRITE_ATTR_ID:
        attr_ntf = RAS_OVER_WRITE_CHAR_SEND;
        break;
    default:
        BT_LOGE("Invalid attr_handle:%d", attr_handle);
        return;
    }

    if (gatts_info->cb->notify_cb) {
        gatts_info->cb->notify_cb(addr, status, attr_ntf);
    }
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
}

bt_status_t ras_gatts_data_send_notify(ras_attr_notify_t attr, bt_address_t* addr,
    uint8_t* value, uint16_t len, bool is_notify)
{
    if (!gatts_info || !value || !addr) {
        BT_LOGE("Invalid params");
        return BT_STATUS_PARM_INVALID;
    }

    uint16_t attr_handle;
    switch (attr) {
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

    bt_status_t status = (is_notify ? (gatts_info->ras_gatts_interface->notify(gatts_info->ras_gatts_handle, addr, attr_handle, value, len)) : gatts_info->ras_gatts_interface->indicate(gatts_info->ras_gatts_handle, addr, attr_handle, value, len));
    return status;
}
