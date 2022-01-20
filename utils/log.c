/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>

#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"
#include "utils/utils.h"
#include "log.h"

static uint8_t  m_framework_log_enable;
static uint8_t  m_log_level;
static uint32_t m_stack_log_mask;

static bool is_support_profile_mask(uint8_t pro)
{
    switch (pro) {
    case SERVICE_DEBUG_HCI:
    case SERVICE_DEBUG_L2CAP:
    case SERVICE_DEBUG_SDP:
    case SERVICE_DEBUG_RFCOMM:
    case SERVICE_DEBUG_ATT:
    case SERVICE_DEBUG_OBEX:
    case SERVICE_DEBUG_AVCTP:
    case SERVICE_DEBUG_AVDTP:
    case SERVICE_DEBUG_AVRCP:
    case SERVICE_DEBUG_SMP:
    case SERVICE_DEBUG_HFP:
    case SERVICE_DEBUG_RAW_PDU:
        return true;
    default:
        return false;
    }
}

static const char* log_id_str(uint8_t id)
{
    switch (id) {
    case LOG_ID_SNOOP:
        return "SNOOP";
    case LOG_ID_STACK:
        return "STACK";
    case LOG_ID_FRAMEWORK:
        return "FRAMEWORK";
    default:
        return "";
    }
}

static const char* profile_mask_str(uint8_t pro)
{
    switch (pro) {
    case SERVICE_DEBUG_HCI:
        return "HCI";
    case SERVICE_DEBUG_L2CAP:
        return "L2CAP";
    case SERVICE_DEBUG_SDP:
        return "SDP";
    case SERVICE_DEBUG_RFCOMM:
        return "RFCOMM";
    case SERVICE_DEBUG_ATT:
        return "ATT";
    case SERVICE_DEBUG_OBEX:
        return "OBEX";
    case SERVICE_DEBUG_AVCTP:
        return "AVCTP";
    case SERVICE_DEBUG_AVDTP:
        return "AVDTP";
    case SERVICE_DEBUG_AVRCP:
        return "AVRCP";
    case SERVICE_DEBUG_SMP:
        return "SMP";
    case SERVICE_DEBUG_HFP:
        return "HFP";
    case SERVICE_DEBUG_RAW_PDU:
        return "RAW_PDU";
    }
    return NULL;
}

static void show_stack_enabled_prifile(uint32_t mask)
{
    char profile[128] = {0};
    uint8_t find = 0;

    for (int i = SERVICE_DEBUG_HCI; i <= SERVICE_DEBUG_RAW_PDU; i++) {
        if (mask & (1 << i)) {
            if (!is_support_profile_mask(i))
                continue;

            const char *pstr = profile_mask_str(i);
            if (find == 0) {
                strcat(profile, pstr);
                find = 1;
            } else {
                strcat(profile, " | ");
                strcat(profile, pstr);
            }
        }
    }

    if (find == 0)
        syslog(LOG_DEBUG, "Enabled Profile Log: NONE\n");
    else
        syslog(LOG_DEBUG, "Enabled Profile Log: %s\n", profile);
}

static void config_stack_log_by_mask(bool en, uint32_t mask)
{
    if (!en) {
        service_adapter_debug_disable(SERVICE_DEBUG_BT);
        return;
    }

    service_adapter_debug_enable(SERVICE_DEBUG_BT);
    for (int i = SERVICE_DEBUG_HCI; i <= SERVICE_DEBUG_RAW_PDU; i++) {
        if (mask & (1 << i) && is_support_profile_mask(i))
            service_adapter_debug_enable(i);
    }
}

void utils_log_init(void)
{
    bool snoop_en, stack_en;

    service_adapter_debug_init();
    //get stack profile or protocol config
    m_stack_log_mask =
        property_get_int32("persist.bluetooth.log.stack", 0x0);
    stack_en = m_stack_log_mask & (1 << SERVICE_DEBUG_BT);
    config_stack_log_by_mask(stack_en, m_stack_log_mask);

    //get snoop log config
    snoop_en = property_get_bool("persist.bluetooth.log.snoop", false);
    if (snoop_en) {
        if (!stack_en)
            service_adapter_debug_enable(SERVICE_DEBUG_BT);
        service_adapter_debug_enable(SERVICE_DEBUG_HCI_DUMP);
    }

    //get framework log level config
    m_log_level =
        property_get_int32("persist.bluetooth.log.level", DEFAULT_BT_LOG_LEVEL);

    m_framework_log_enable = 1;
    syslog(LOG_DEBUG, "Log Module: SNOOP:%d, STACK:%d, FRAMEWORK:%d\n", snoop_en, stack_en, m_framework_log_enable);
    if (stack_en)
        show_stack_enabled_prifile(m_stack_log_mask);
}

int utils_log_enable(int id)
{
    switch (id) {
    case LOG_ID_SNOOP:
        //if (!(m_stack_log_mask & (1 << SERVICE_DEBUG_BT)))
        //    service_adapter_debug_enable(SERVICE_DEBUG_BT);
        service_adapter_debug_enable(SERVICE_DEBUG_HCI_DUMP);
        property_set_bool("persist.bluetooth.log.snoop", true);
        break;
    case LOG_ID_STACK:
        //service_adapter_debug_enable(SERVICE_DEBUG_BT);
        m_stack_log_mask |= (1 << SERVICE_DEBUG_BT) & 0xFFFFFFFF;
        config_stack_log_by_mask(true, m_stack_log_mask);
        property_set_int32("persist.bluetooth.log.stack", m_stack_log_mask);
        break;
    case LOG_ID_FRAMEWORK:
        m_framework_log_enable = 1;
        break;
    default:
        return -1;
    }
    syslog(LOG_DEBUG, "%s Log Enabled\n", log_id_str(id));

    return 0;
}

int utils_log_disable(int id)
{
    switch (id) {
    case LOG_ID_SNOOP:
        //if (!(m_stack_log_mask & (1 << SERVICE_DEBUG_BT)))
        //    service_adapter_debug_disable(SERVICE_DEBUG_BT);
        //else
        service_adapter_debug_disable(SERVICE_DEBUG_HCI_DUMP);
        property_set_bool("persist.bluetooth.log.snoop", false);
        break;
    case LOG_ID_STACK:
        service_adapter_debug_disable(SERVICE_DEBUG_BT);
        m_stack_log_mask &= ~((1 << SERVICE_DEBUG_BT) & 0xFFFFFFFF);
        property_set_int32("persist.bluetooth.log.stack", m_stack_log_mask);
        break;
    case LOG_ID_FRAMEWORK:
        m_framework_log_enable = 0;
        break;
    default:
        return -1;
    }
    syslog(LOG_DEBUG, "%s Log Disabled\n", log_id_str(id));

    return 0;
}

uint8_t utils_set_log_level(uint8_t level)
{
    if (level > BT_LOG_LEVEL_DEBUG)
        level = BT_LOG_LEVEL_DEBUG;

    m_log_level = level;
    property_set_int32("persist.bluetooth.log.level", level);

    return m_log_level;
}

int utils_set_log_mask_level(uint8_t id, uint8_t mask_bit, bool enable)
{
    if (id == LOG_ID_STACK) {
        if (is_support_profile_mask(mask_bit)) {
            if (enable) {
                service_adapter_debug_enable(mask_bit);
                m_stack_log_mask |= (1 << mask_bit) & 0xFFFFFFFF;
            } else {
                service_adapter_debug_disable(mask_bit);
                m_stack_log_mask &= ~((1 << mask_bit) & 0xFFFFFFFF);
            }

            property_set_int32("persist.bluetooth.log.stack", m_stack_log_mask);
            syslog(LOG_DEBUG, "%s Log %s\n", profile_mask_str(mask_bit), enable ? "Enabled" : "Disabled");
            return 0;
        }
    }

    return -1;
}

uint8_t utils_get_log_level(void)
{
    return m_log_level;
}

bool utils_log_print_check(uint8_t level)
{
    if (m_log_level < level ||
        m_log_level == BT_LOG_LEVEL_OFF ||
        m_framework_log_enable == 0)
        return false;

    return true;
}

