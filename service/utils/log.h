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

#ifndef __BT_LOG_H__
#define __BT_LOG_H__

#include <stdarg.h>
#include <syslog.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#ifndef LOG_TAG
#define LOG_TAG "BT"
#endif

#define _S_LINE(x)  #x
#define __S_LINE(x) _S_LINE(x)
#define __S_LINE__  __S_LINE(__LINE__)

#define LOG_ID_SNOOP     0
#define LOG_ID_STACK     1
#define LOG_ID_FRAMEWORK 2

enum bt_log_level_ {
    BT_LOG_LEVEL_OFF = 0x0,
    BT_LOG_LEVEL_ERROR = LOG_ERR,
    BT_LOG_LEVEL_WARNING = LOG_WARNING,
    BT_LOG_LEVEL_INFO = LOG_INFO,
    BT_LOG_LEVEL_DEBUG = LOG_DEBUG,
};

#ifndef CONFIG_OBELISK_LOG_LEVEL
#define DEFAULT_BT_LOG_LEVEL BT_LOG_LEVEL_OFF
#define BT_LOG(id, level, fmt, ...)
#define BT_LOGE(fmt, args...)
#define BT_LOGW(fmt, args...)
#define BT_LOGI(fmt, args...)
#define BT_LOGD(fmt, args...)
#else
extern bool utils_log_print_check(uint8_t level);

#define DEFAULT_BT_LOG_LEVEL CONFIG_OBELISK_LOG_LEVEL

#define BT_LOG(id, level, fmt, args...) syslog(level, "["__S_LINE__   \
                                                      "]"             \
                                                      "[" LOG_TAG "]" \
                                                      ": " fmt "\n",  \
                                               ##args);
#define BT_LOGE(fmt, ...)                                                     \
    do {                                                                      \
        if (utils_log_print_check(BT_LOG_LEVEL_ERROR))                        \
            BT_LOG(LOG_ID_FRAMEWORK, BT_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__); \
    } while (0);
#define BT_LOGW(fmt, ...)                                                       \
    do {                                                                        \
        if (utils_log_print_check(BT_LOG_LEVEL_WARNING))                        \
            BT_LOG(LOG_ID_FRAMEWORK, BT_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__); \
    } while (0);
#define BT_LOGI(fmt, ...)                                                    \
    do {                                                                     \
        if (utils_log_print_check(BT_LOG_LEVEL_INFO))                        \
            BT_LOG(LOG_ID_FRAMEWORK, BT_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__); \
    } while (0);
#define BT_LOGD(fmt, ...)                                                     \
    do {                                                                      \
        if (utils_log_print_check(BT_LOG_LEVEL_DEBUG))                        \
            BT_LOG(LOG_ID_FRAMEWORK, BT_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__); \
    } while (0);
#endif

#define BT_ADDR_LOG(fmt, _addr, ...)                \
    do {                                           \
        char _addr_str[BT_ADDR_STR_LENGTH] = { 0 }; \
        bt_addr_ba2str(_addr, _addr_str);            \
        BT_LOGD(fmt, _addr_str, ##__VA_ARGS__);     \
    } while (0);

void utils_log_init(void);
int utils_log_enable(int id);
int utils_log_disable(int id);
uint8_t utils_set_log_level(uint8_t level);
uint8_t utils_get_log_level(void);
int utils_set_log_mask_level(uint8_t id, uint8_t mask_bit, bool enable);

#endif
