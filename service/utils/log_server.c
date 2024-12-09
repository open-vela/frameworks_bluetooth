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

#include "bt_status.h"
#include "btsnoop_log.h"
#include "log.h"
#include "sal_interface.h"
#include "service_loop.h"

enum {
    FRAMEWORK_LOG_LEVEL_CHANGED,
    STACK_LOG_EN_CHANGED,
    STACK_LOG_MASK_CHANGED,
    SNOOP_LOG_EN_CHANGED,
};

#define PERSIST_BT_LOG_CHANGED "persist.bluetooth.log.changed"
#define PERSIST_BT_FRAMEWORK_LOG_LEVEL "persist.bluetooth.log.level"
#define PERSIST_BT_STACK_LOG_EN "persist.bluetooth.log.stack_enable"
#define PERSIST_BT_STACK_LOG_MASK "persist.bluetooth.log.stack_mask"
#define PERSIST_BT_SNOOP_LOG_EN "persist.bluetooth.log.snoop_enable"
#define PERSIST_BT_SNOOP_FILE_PATH "persist.bluetooth.log.snoop_path"

// #define PERSIST_BT_SNOOP_LOG_CID_MASK "persist.bluetooth.log.snoop_cid_mask"
// #define PERSIST_BT_SNOOP_LOG_PKT_MASK "persist.bluetooth.log.snoop_pkt_mask"

struct bt_logger {
    uint8_t stack_enable;
    uint8_t snoop_enable;

    uint8_t framework_level;
    int stack_mask;
    int snoop_cid_mask;
    int snoop_pkt_mask;

    int monitor_fd;
    service_poll_t* poll;
};

static struct bt_logger g_logger = { 0, 0, BT_LOG_LEVEL_OFF, 0, 0, 0, -1 };

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

static uint8_t bt_log_set_level(uint8_t level, bool changed)
{
    if (level != BT_LOG_LEVEL_OFF && level != BT_LOG_LEVEL_ERROR && level != BT_LOG_LEVEL_WARNING && level != BT_LOG_LEVEL_INFO && level != BT_LOG_LEVEL_DEBUG)
        return g_logger.framework_level;

    g_logger.framework_level = level;

#if defined(CONFIG_KVDB) && defined(__NuttX__)
    if (!changed) {
        property_set_int32(PERSIST_BT_FRAMEWORK_LOG_LEVEL, level);
        property_commit();
    }
#endif
    syslog(LOG_INFO, "framework log level: %d\n", g_logger.framework_level);

    return g_logger.framework_level;
}

static int stack_log_setup(void)
{
    if (bt_sal_debug_enable() != BT_STATUS_SUCCESS) {
        syslog(LOG_ERR, "%s\n", "stack log not support");
        return -1;
    }

    if (bt_sal_debug_update_log_mask(g_logger.stack_mask) != BT_STATUS_SUCCESS) {
        syslog(LOG_ERR, "%s\n", "stack log mask update fail");
        bt_sal_debug_disable();
        return -1;
    }

    return 0;
}

void bt_log_module_enable(int id, bool changed)
{
    const char* property = NULL;
    syslog(LOG_DEBUG, "%s, id %d\n", __func__, id);
    switch (id) {
    case LOG_ID_SNOOP: {
        if (g_logger.snoop_enable)
            return;

        if (btsnoop_log_enable() != BT_STATUS_SUCCESS) {
            syslog(LOG_ERR, "%s\n", "enable snoop log fail");
            return;
        }
        g_logger.snoop_enable = 1;
        property = PERSIST_BT_SNOOP_LOG_EN;
        break;
    }
    case LOG_ID_STACK: {
        if (g_logger.stack_enable)
            return;

        if (stack_log_setup() != 0)
            return;

        g_logger.stack_enable = 1;
        property = PERSIST_BT_STACK_LOG_EN;
        break;
    }
    case LOG_ID_FRAMEWORK: {
        return;
    }
    }

#if defined(CONFIG_KVDB) && defined(__NuttX__)
    if (!changed) {
        property_set_int32(property, 1);
        property_commit();
    }
#endif
    syslog(LOG_INFO, "%s enabled\n", log_id_str(id));
}

void bt_log_module_disable(int id, bool changed)
{
    const char* property = NULL;
    syslog(LOG_DEBUG, "%s id %d\n", __func__, id);
    switch (id) {
    case LOG_ID_SNOOP: {
        if (!g_logger.snoop_enable)
            return;

        btsnoop_log_disable();
        g_logger.snoop_enable = 0;
        property = PERSIST_BT_SNOOP_LOG_EN;
        break;
    }
    case LOG_ID_STACK: {
        if (!g_logger.stack_enable)
            return;

        bt_sal_debug_disable();
        g_logger.stack_enable = 0;
        property = PERSIST_BT_STACK_LOG_EN;
        break;
    }
    case LOG_ID_FRAMEWORK: {
        g_logger.framework_level = BT_LOG_LEVEL_OFF;
        return;
    }
    }

#if defined(CONFIG_KVDB) && defined(__NuttX__)
    if (!changed) {
        property_set_int32(property, 0);
        property_commit();
    }
#endif
    syslog(LOG_INFO, "%s disabled\n", log_id_str(id));
}

static void property_monitor_cb(service_poll_t* poll,
    int revent, void* userdata)
{
    if (revent & POLL_ERROR || revent & POLL_DISCONNECT) {
        service_loop_remove_poll(g_logger.poll);
        g_logger.poll = NULL;
        if (g_logger.monitor_fd)
            property_monitor_close(g_logger.monitor_fd);
        g_logger.monitor_fd = -1;
    } else if (revent & POLL_READABLE) {
        int changed = 0;
        char key[PROP_NAME_MAX];
        int new;

        property_monitor_read(g_logger.monitor_fd, key, &changed, sizeof(changed));
        if (changed & (1 << FRAMEWORK_LOG_LEVEL_CHANGED)) {
            new = property_get_int32(PERSIST_BT_FRAMEWORK_LOG_LEVEL, 0);
            if (new != g_logger.framework_level)
                bt_log_set_level(new, true);
        }

        if (changed & (1 << STACK_LOG_EN_CHANGED)) {
            new = property_get_int32(PERSIST_BT_STACK_LOG_EN, 0);
            if (new != g_logger.stack_enable) {
                if (new)
                    bt_log_module_enable(LOG_ID_STACK, true);
                else
                    bt_log_module_disable(LOG_ID_STACK, true);
            }
        }

        if (changed & (1 << STACK_LOG_MASK_CHANGED)) {
            new = property_get_int32(PERSIST_BT_STACK_LOG_MASK, 0);
            if (new != g_logger.stack_mask) {
                g_logger.stack_mask = new;
                bt_sal_debug_update_log_mask(g_logger.stack_mask);
            }
        }

        if (changed & (1 << SNOOP_LOG_EN_CHANGED)) {
            new = property_get_int32(PERSIST_BT_SNOOP_LOG_EN, 0);
            if (new != g_logger.snoop_enable) {
                if (new)
                    bt_log_module_enable(LOG_ID_SNOOP, true);
                else
                    bt_log_module_disable(LOG_ID_SNOOP, true);
            }
        }
    }
}

void bt_log_server_init(void)
{
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    char path[SNOOP_PATH_MAX_LEN];

    /** framework log init */
    g_logger.framework_level = property_get_int32(PERSIST_BT_FRAMEWORK_LOG_LEVEL, DEFAULT_BT_LOG_LEVEL);

    /** stack log init */
    bt_sal_debug_init();
    g_logger.stack_enable = property_get_int32(PERSIST_BT_STACK_LOG_EN, 0);
    g_logger.stack_mask = property_get_int32(PERSIST_BT_STACK_LOG_MASK, 0);
    if (g_logger.stack_enable)
        stack_log_setup();

    if (property_get_binary(PERSIST_BT_SNOOP_FILE_PATH, path, SNOOP_PATH_MAX_LEN) <= 0) {
        strlcpy(path, CONFIG_BLUETOOTH_SNOOP_LOG_DEFAULT_PATH, SNOOP_PATH_MAX_LEN);
    }
    /** snoop log init */
    if (btsnoop_log_init(path) != BT_STATUS_SUCCESS)
        syslog(LOG_ERR, "init snoop log fail\n");

    g_logger.snoop_enable = property_get_int32(PERSIST_BT_SNOOP_LOG_EN, 0);
    if (g_logger.snoop_enable) {
        if (btsnoop_log_enable() != BT_STATUS_SUCCESS)
            syslog(LOG_ERR, "%s\n", "enable snoop log fail");
    }

    /** start log property monitor */
    property_set_int32(PERSIST_BT_LOG_CHANGED, 0);
    g_logger.monitor_fd = property_monitor_open(PERSIST_BT_LOG_CHANGED);
    if (g_logger.monitor_fd < 0) {
        syslog(LOG_ERR, "propert monitor open fail: %d\n", errno);
        return;
    }

    g_logger.poll = service_loop_poll_fd(g_logger.monitor_fd, POLL_READABLE, property_monitor_cb, NULL);
    if (g_logger.poll == NULL)
        syslog(LOG_ERR, "%s\n", "propert monitor poll error");

    syslog(1, "Framework log level: %d, Stack:%d, mask:%08x, Snoop: %d\n", g_logger.framework_level,
        g_logger.stack_enable, g_logger.stack_mask, g_logger.snoop_enable);
#else
#endif
}

void bt_log_server_cleanup(void)
{
    /** stop log property monitor */
    service_loop_remove_poll(g_logger.poll);
    g_logger.poll = NULL;
    if (g_logger.monitor_fd)
        property_monitor_close(g_logger.monitor_fd);

    g_logger.monitor_fd = -1;

    /** snoop log deinit */
    if (g_logger.snoop_enable)
        btsnoop_log_disable();

    btsnoop_log_uninit();

    /** stack log deinit */
    if (g_logger.stack_enable)
        bt_sal_debug_disable();
    bt_sal_debug_cleanup();
}

bool bt_log_print_check(uint8_t level)
{
    if (g_logger.framework_level < level || g_logger.framework_level == BT_LOG_LEVEL_OFF)
        return false;

    return true;
}