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

#ifndef __BT_LOG_H__
#define __BT_LOG_H__

#include <debug.h>
#include <stdarg.h>
#include <syslog.h>

#ifndef LOG_TAG
#define LOG_TAG "BT"
#endif

#define _S_LINE(x) #x
#define __S_LINE(x) _S_LINE(x)
#define __S_LINE__ __S_LINE(__LINE__)

#define LOG_ID_SNOOP 0
#define LOG_ID_STACK 1
#define LOG_ID_FRAMEWORK 2

enum bt_log_level_ {
    BT_LOG_LEVEL_OFF = 0x0,
    BT_LOG_LEVEL_ERROR = LOG_ERR,
    BT_LOG_LEVEL_WARNING = LOG_WARNING,
    BT_LOG_LEVEL_INFO = LOG_INFO,
    BT_LOG_LEVEL_DEBUG = LOG_DEBUG,
};

#ifndef CONFIG_BT_FRAMEWORK_LOG_LEVEL
#define DEFAULT_BT_LOG_LEVEL BT_LOG_LEVEL_OFF
#define BT_LOG(id, level, fmt, ...)
#define BT_LOGE(fmt, args...)
#define BT_LOGW(fmt, args...)
#define BT_LOGI(fmt, args...)
#define BT_LOGD(fmt, args...)
#else
extern bool utils_log_print_check(uint8_t level);

#define DEFAULT_BT_LOG_LEVEL CONFIG_BT_FRAMEWORK_LOG_LEVEL

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

#define BT_ADDR_LOGD(fmt, args...) syslog(6, LOG_TAG ": " fmt "\n", ##args)

#define BT_HEXDUMP(array, size) lib_dumpbuffer("BT_HEXDUMP: ", array, (uint32_t)size)

void utils_log_init(void);
int utils_log_enable(int id);
int utils_log_disable(int id);
uint8_t utils_set_log_level(uint8_t level);
uint8_t utils_get_log_level(void);
int utils_set_log_mask_level(uint8_t id, uint8_t mask_bit, bool enable);

#endif