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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_gap.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"

struct _debug_type_map {
    bt_debug_type_t service_type;
    SERVICE_DEBUG_COMPONENT stack_type;
} g_dbg_type_map[] = {
    {BT_DBG_TYPE_HCI,       SERVICE_DEBUG_HCI     },
    { BT_DBG_TYPE_HCI_RAW,  SERVICE_DEBUG_RAW_PDU },
    { BT_DBG_TYPE_HCI_DUMP, SERVICE_DEBUG_HCI_DUMP},
    { BT_DBG_TYPE_L2CAP,    SERVICE_DEBUG_L2CAP   },
    { BT_DBG_TYPE_SDP,      SERVICE_DEBUG_SDP     },
    { BT_DBG_TYPE_ATT,      SERVICE_DEBUG_ATT     },
    { BT_DBG_TYPE_SMP,      SERVICE_DEBUG_SMP     },
    { BT_DBG_TYPE_RFCOMM,   SERVICE_DEBUG_RFCOMM  },
    { BT_DBG_TYPE_OBEX,     SERVICE_DEBUG_OBEX    },
    { BT_DBG_TYPE_AVCTP,    SERVICE_DEBUG_AVCTP   },
    { BT_DBG_TYPE_AVDTP,    SERVICE_DEBUG_AVDTP   },
    { BT_DBG_TYPE_AVRCP,    SERVICE_DEBUG_AVRCP   },
    { BT_DBG_TYPE_A2DP,     0                     },
    { BT_DBG_TYPE_HFP,      SERVICE_DEBUG_HFP     },
};

#ifdef CONFIG_BLUELET_DEBUG
static SERVICE_DEBUG_COMPONENT get_component_value(bt_debug_type_t type)
{
    for (int i = 0; i < ARRAY_SIZE(g_dbg_type_map); i++) {
        if (g_dbg_type_map[i].service_type == type)
            return g_dbg_type_map[i].stack_type;
    }

    return 0;
}
#endif

static uint32_t support_types(void)
{
    uint32_t mask = 0;

#ifdef CONFIG_BLUELET_DEBUG
    mask = mask
#if CONFIG_BLUELET_DBG_HCI
           | (1 << BT_DBG_TYPE_HCI)
#endif
#ifdef CONFIG_BLUELET_DBG_HCIRAW
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_L2CAP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_SDP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_ATT
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_SMP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_RFCOMM
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_OBEX
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_AVCTP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_AVDTP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_AVRCP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
#ifdef CONFIG_BLUELET_DBG_HFP
           | (1 << BT_DBG_TYPE_HCI_RAW)
#endif
        ;
#endif

    return mask;
}

void bt_sal_debug_init(void)
{
    service_adapter_debug_init();
}

void bt_sal_debug_cleanup(void)
{
}

bt_status_t bt_sal_debug_enable(void)
{
#ifdef CONFIG_BLUELET_DEBUG
    service_adapter_debug_enable(SERVICE_DEBUG_BT);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_debug_disable(void)
{
#ifdef CONFIG_BLUELET_DEBUG
    service_adapter_debug_disable(SERVICE_DEBUG_BT);
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_debug_set_log_level(uint32_t level)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bool bt_sal_debug_is_type_support(bt_debug_type_t type)
{
    return support_types() & (1 << type);
}

bt_status_t bt_sal_debug_set_log_enable(bt_debug_type_t type, bool enable)
{
#ifdef CONFIG_BLUELET_DEBUG
    SERVICE_DEBUG_COMPONENT cp_type;

    if (!bt_sal_debug_is_type_support(type))
        return BT_STATUS_NOT_SUPPORTED;

    cp_type = get_component_value(type);
    if (cp_type == 0)
        return BT_STATUS_NOT_SUPPORTED;

    if (enable)
        service_adapter_debug_enable(cp_type);
    else
        service_adapter_debug_disable(cp_type);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t bt_sal_debug_update_log_mask(int mask)
{
#ifdef CONFIG_BLUELET_DEBUG
    SERVICE_DEBUG_COMPONENT cp_type;

    for (int i = 0; i < ARRAY_SIZE(g_dbg_type_map); i++) {
        if (g_dbg_type_map[i].stack_type == 0)
            continue;

        cp_type = g_dbg_type_map[i].stack_type;
        if (((1 << g_dbg_type_map[i].service_type) & 0xFFFFFFFF) & mask)
            service_adapter_debug_enable(cp_type);
        else
            service_adapter_debug_disable(cp_type);
    }

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}
