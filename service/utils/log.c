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
#include <string.h>
#include <syslog.h>

#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "log.h"

static uint8_t m_framework_log_enable;
static uint8_t m_log_level;

static const char *log_id_str(uint8_t id)
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

void utils_log_init(void)
{
    // get framework log level config
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    m_log_level = property_get_int32("persist.bluetooth.log.level", DEFAULT_BT_LOG_LEVEL);
#else
    m_log_level = BT_LOG_LEVEL_OFF;
#endif

    m_framework_log_enable = 1;
    syslog(LOG_DEBUG, "Log Module: FRAMEWORK:%d\n", m_framework_log_enable);
}

int utils_log_enable(int id)
{
    switch (id) {
    case LOG_ID_FRAMEWORK:
        m_framework_log_enable = 1;
        break;
    default:
        return -1;
    }

#if defined(CONFIG_KVDB) && defined(__NuttX__)
    property_commit();
#endif
    syslog(LOG_DEBUG, "%s Log Enabled\n", log_id_str(id));

    return 0;
}

int utils_log_disable(int id)
{
    switch (id) {
    case LOG_ID_FRAMEWORK:
        m_framework_log_enable = 0;
        break;
    default:
        return -1;
    }
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    property_commit();
#endif
    syslog(LOG_DEBUG, "%s Log Disabled\n", log_id_str(id));

    return 0;
}

uint8_t utils_set_log_level(uint8_t level)
{
    if (level > BT_LOG_LEVEL_DEBUG)
        level = BT_LOG_LEVEL_DEBUG;

    m_log_level = level;
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    property_set_int32("persist.bluetooth.log.level", level);
    property_commit();
#endif

    return m_log_level;
}

uint8_t utils_get_log_level(void)
{
    return m_log_level;
}

bool utils_log_print_check(uint8_t level)
{
    if (m_log_level < level || m_log_level == BT_LOG_LEVEL_OFF || m_framework_log_enable == 0)
        return false;

    return true;
}
