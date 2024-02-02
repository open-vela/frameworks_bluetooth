/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#include <stdint.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_l2cap_interface.h"

#ifdef CONFIG_BLUETOOTH_L2CAP

static SERVICE_L2CAP_MODE_TYPE bluelet_l2cap_mode(l2cap_channel_mode_t mode)
{
    switch (mode) {
    case L2CAP_CHANNEL_MODE_BASIC:
        return SERVICE_L2CAP_MODE_BASIC;
    case L2CAP_CHANNEL_MODE_RETRANSMISSION:
        return SERVICE_L2CAP_MODE_RTX;
    case L2CAP_CHANNEL_MODE_FLOW_CONTROL:
        return SERVICE_L2CAP_MODE_FC;
    case L2CAP_CHANNEL_MODE_ENHANCED_RETRANSMISSION:
        return SERVICE_L2CAP_MODE_ERTX;
    case L2CAP_CHANNEL_MODE_STREAMING_MODE:
        return SERVICE_L2CAP_MODE_SM;
    case L2CAP_CHANNEL_MODE_LE_CREDIT_BASED_FLOW_CONTROL:
        return SERVICE_L2CAP_MODE_LE_CREDIT_BASED_FLOW_CONTROL;
    case L2CAP_CHANNEL_MODE_ENHANCED_CREDIT_BASED_FLOW_CONTROL:
        return SERVICE_L2CAP_MODE_ENHANCED_CREDIT_BASED_FLOW_CONTROL;
    default:
        BT_LOGE("Unknow l2cap channel mode: %d", mode);
        return SERVICE_L2CAP_MODE_BASIC;
    }
}

bt_status_t bt_sal_l2cap_listen_channel(l2cap_config_option_t *option)
{
    SAL_CHECK_PARAM(option);

    if (option->transport == BT_TRANSPORT_BLE) {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        SERVICE_L2CAP_CONFIG_OPTION_S le_l2cap_config_opt = {
            .psm = option->psm,
            .mode = bluelet_l2cap_mode(option->mode),
            .incoming.mtu = option->mtu,
            .incoming.le_mps = option->le_mps,
            .incoming.credits = option->init_credits,
        };

        SAL_CHECK_RET(service_adapter_gap_ble_listen_l2cap_channel(&le_l2cap_config_opt),
                      SERVICE_BT_STATUS_SUCCESS);
#else
        return BT_STATUS_NOT_SUPPORTED;
#endif
    } else if (option->transport == BT_TRANSPORT_BREDR) {
        return BT_STATUS_NOT_SUPPORTED;
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_connect_channel(bt_address_t *addr, l2cap_config_option_t *option)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(option);

    if (option->transport == BT_TRANSPORT_BLE) {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        SERVICE_L2CAP_CONFIG_OPTION_S le_l2cap_config_opt = {
            .psm = option->psm,
            .mode = bluelet_l2cap_mode(option->mode),
            .incoming.mtu = option->mtu,
            .incoming.le_mps = option->le_mps,
            .incoming.credits = option->init_credits,
        };

        SAL_CHECK_RET(service_adapter_gap_ble_connect_l2cap_channel(addr->addr, &le_l2cap_config_opt),
                      SERVICE_BT_STATUS_SUCCESS);
#else
        return BT_STATUS_NOT_SUPPORTED;
#endif
    } else if (option->transport == BT_TRANSPORT_BREDR) {
        return BT_STATUS_NOT_SUPPORTED;
    } else {
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_disconnect_channel(uint16_t cid)
{
    SAL_CHECK_RET(service_adapter_gap_disconnect_l2cap_channel(cid), SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_l2cap_send_packet(uint16_t cid, uint8_t *packet_data, uint16_t packet_size)
{
    SAL_CHECK_RET(service_adapter_gap_send_l2cap_packet(cid, packet_data, packet_size),
                  SERVICE_BT_STATUS_SUCCESS);

    return BT_STATUS_SUCCESS;
}

#endif
