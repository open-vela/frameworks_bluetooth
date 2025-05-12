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
#include <nuttx/list.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "app_bt_message_gap.h"
#include "bluetooth.h"
#include "bt_adapter.h"

typedef enum {
#define __APP_BT_MESSAGE_CODE__
#include "app_bt_message_gap.h"
#undef __APP_BT_MESSAGE_CODE__
} app_bt_message_type_t;

#define APP_LOG_TAG "app_demo"

#define LOG_EMERG 0 /* system is unusable */
#define LOG_ALERT 1 /* action must be taken immediately */
#define LOG_CRIT 2 /* critical conditions */
#define LOG_ERR 3 /* error conditions */
#define LOG_WARNING 4 /* warning conditions */
#define LOG_NOTICE 5 /* normal but significant condition */
#define LOG_INFO 6 /* informational */
#define LOG_DEBUG 7 /* debug-level messages */

#define LOGE(fmt, ...) syslog(LOG_ERR, "[" APP_LOG_TAG "]" \
                                       ": " fmt "\n",      \
    ##__VA_ARGS__);
#define LOGW(fmt, ...) syslog(LOG_WARNING, "[" APP_LOG_TAG "]" \
                                           ": " fmt "\n",      \
    ##__VA_ARGS__);
#define LOGI(fmt, ...) syslog(LOG_INFO, "[" APP_LOG_TAG "]" \
                                        ": " fmt "\n",      \
    ##__VA_ARGS__);

typedef struct {
    uint32_t msg_type;
    union {
        app_bt_message_gap_t gap_req;
    };
    union {
        app_bt_message_gap_callbacks_t gap_cb;
    };
} app_demo_message_t;

typedef struct {
    struct list_node node;
    app_demo_message_t data;
} node_t;

typedef struct {
    uint8_t running;
    sem_t sem;
    pthread_mutex_t mutex;
    struct list_node message_queue;
} app_demo_t;

void demo_discovery_handle_gap_message(bt_instance_t* g_bt_ins, node_t* node);