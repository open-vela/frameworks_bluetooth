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
#include <pthread.h>
#include <syslog.h>

#include "btsnoop_filter.h"
#include "btsnoop_log.h"
#include "btsnoop_writer.h"
#include "log.h"

#ifndef CONFIG_BLUETOOTH_SNOOP_LOG
#define CONFIG_BLUETOOTH_SNOOP_LOG 1
#endif

static pthread_mutex_t snoop_lock = PTHREAD_MUTEX_INITIALIZER;
static bool snoop_enable = false;

void btsnoop_log_capture(uint8_t recieve, uint8_t* hci_pkt, uint32_t hci_pkt_size)
{
#if CONFIG_BLUETOOTH_SNOOP_LOG
    pthread_mutex_lock(&snoop_lock);

    if (!snoop_enable) {
        pthread_mutex_unlock(&snoop_lock);
        return;
    }
    if (filter_can_filter(recieve, hci_pkt, hci_pkt_size)) {
        pthread_mutex_unlock(&snoop_lock);
        return;
    }

    pthread_mutex_unlock(&snoop_lock);
    writer_write_log(recieve, hci_pkt, hci_pkt_size);
#endif
}

int btsnoop_log_init(char* path)
{
    if (pthread_mutex_init(&snoop_lock, NULL) < 0)
        return BT_STATUS_FAIL;

    set_snoop_file_path(path);
    return BT_STATUS_SUCCESS;
}

void btsnoop_log_uninit(void)
{
    pthread_mutex_destroy(&snoop_lock);
}

int btsnoop_log_enable(void)
{
#if CONFIG_BLUETOOTH_SNOOP_LOG
    pthread_mutex_lock(&snoop_lock);
    if (writer_init() < 0) {
        syslog(LOG_ERR, "%s fail", __func__);
        pthread_mutex_unlock(&snoop_lock);
        return BT_STATUS_FAIL;
    }

    if (filter_init() < 0) {
        syslog(LOG_ERR, "%s fail", __func__);
        pthread_mutex_unlock(&snoop_lock);
        return BT_STATUS_FAIL;
    }

    snoop_enable = true;

    pthread_mutex_unlock(&snoop_lock);
    return BT_STATUS_SUCCESS;
#else
    syslog(LOG_WARNING, "%s\n", "CONFIG_BLUETOOTH_SNOOP_LOG not set");
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void btsnoop_log_disable(void)
{
#if CONFIG_BLUETOOTH_SNOOP_LOG
    pthread_mutex_lock(&snoop_lock);
    snoop_enable = false;
    filter_uninit();
    writer_uninit();
    pthread_mutex_unlock(&snoop_lock);
#endif
}

int btsnoop_set_filter(btsnoop_filter_flag_t filter_flag)
{
    return filter_set_filter_flag(filter_flag);
}

int btsnoop_remove_filter(btsnoop_filter_flag_t filter_flag)
{
    return filter_remove_filter_flag(filter_flag);
}