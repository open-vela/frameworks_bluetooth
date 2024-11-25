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

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>

#include "bt_time.h"
#include "btsnoop_log.h"

#ifndef CONFIG_BLUETOOTH_SNOOP_LOG
#define CONFIG_BLUETOOTH_SNOOP_LOG 1
#endif

#ifdef CONFIG_BLUETOOTH_SNOOP_LOG
#define CONFIG_BLUETOOTH_SNOOP_LOG_PATH "/data/misc/bt/snoop"
#endif

struct btsnoop_file_hdr {
    uint8_t id[8]; /* Identification Pattern */
    uint32_t version; /* Version Number = 1 */
    uint32_t type; /* Datalink Type */
};

struct btsnoop_pkt_hdr {
    uint32_t size; /* Original Length */
    uint32_t len; /* Included Length */
    uint32_t flags; /* Packet Flags: 1 hci cmd */
    uint32_t drops; /* Cumulative Drops */
    uint64_t ts; /* Timestamp microseconds */
    //  uint8_t       data[0];        /* Packet Data */
};

static int snoop_fd = -1;
static time_t time_base;
static uint32_t ms_base;
static pthread_mutex_t snoop_lock = PTHREAD_MUTEX_INITIALIZER;

static uint32_t get_current_time_ms(void)
{
    return (uint32_t)(get_os_timestamp_us() / 1000);
}

static unsigned long byteswap_ulong(unsigned long val)
{
    unsigned char* byte_val = (unsigned char*)&val;
    return ((unsigned long)byte_val[3] + ((unsigned long)byte_val[2] << 8) + ((unsigned long)byte_val[1] << 16) + ((unsigned long)byte_val[0] << 24));
}

static void btsnoop_write_log(uint8_t is_recieve, uint8_t* p, uint32_t len)
{
    struct btsnoop_pkt_hdr pkt;
    uint32_t ms;
    int ret;

    if (snoop_fd < 0)
        return;

    ms = get_current_time_ms() - ms_base;
    const uint64_t sec = (uint32_t)(time_base + ms / 1000 + 8 * 3600);
    const uint64_t usec = (uint32_t)((ms % 1000) * 1000);
    uint64_t nts = (sec - (int64_t)946684800) * (int64_t)1000000 + usec;
    uint32_t* d = (uint32_t*)&pkt.ts;
    uint32_t* s = (uint32_t*)&nts;

    pkt.size = byteswap_ulong(len);
    pkt.len = pkt.size;
    pkt.drops = 0;
    pkt.flags = (is_recieve) ? byteswap_ulong(0x01) : 0;
    nts += (0x4A676000) + (((int64_t)0x00E03AB4) << 32);
    d[0] = byteswap_ulong(s[1]);
    d[1] = byteswap_ulong(s[0]);
    ret = write(snoop_fd, &pkt, sizeof(pkt));
    if (ret < 0)
        syslog(LOG_ERR, "snoop log header write ret:%d, error:%d\n", ret, errno);

    ret = write(snoop_fd, p, len);
    if (ret < 0)
        syslog(LOG_ERR, "snoop log data write ret:%d, error:%d\n", ret, errno);

    fsync(snoop_fd);
}

int btsnoop_create_new_file(void)
{
    struct btsnoop_file_hdr hdr;
    time_t rawtime;
    struct tm* info;
    char ts_str[80];
    char file_name[128];
    int ret;

    pthread_mutex_lock(&snoop_lock);
    if (snoop_fd > 0) {
        close(snoop_fd);
        snoop_fd = -1;
    }

    if (-1 == mkdir(CONFIG_BLUETOOTH_SNOOP_LOG_PATH, 0777) && errno != EEXIST) {
        syslog(LOG_ERR, "snoop folder create fail:%d", errno);
        pthread_mutex_unlock(&snoop_lock);
        return -errno;
    }

    time_base = time(NULL);
    ms_base = get_current_time_ms();

    time(&rawtime);
    info = localtime(&rawtime);
    if (info == NULL) {
        pthread_mutex_unlock(&snoop_lock);
        return -1;
    }

    snprintf(ts_str, sizeof(ts_str), "%d%02d%02d_%02d%02d%02d",
        info->tm_year + 1900,
        info->tm_mon + 1,
        info->tm_mday,
        info->tm_hour,
        info->tm_min,
        info->tm_sec);
    snprintf(file_name, sizeof(file_name), CONFIG_BLUETOOTH_SNOOP_LOG_PATH "/snoop_%s_%" PRIu32 ".log", ts_str, ms_base);

    ret = open(file_name, O_RDWR | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (ret < 0) {
        snoop_fd = -1;
        pthread_mutex_unlock(&snoop_lock);
        return ret;
    }

    snoop_fd = ret;
    memcpy(hdr.id, "btsnoop", 8);
    hdr.version = byteswap_ulong(1);
    hdr.type = byteswap_ulong(1002);
    ret = write(snoop_fd, &hdr, sizeof(hdr));
    pthread_mutex_unlock(&snoop_lock);
    return ret;
}

void btsnoop_close_file(void)
{
    pthread_mutex_lock(&snoop_lock);
    if (snoop_fd > 0) {
        fsync(snoop_fd);
        close(snoop_fd);
        snoop_fd = -1;
    }
    pthread_mutex_unlock(&snoop_lock);
}

bt_status_t btsnoop_log_open(void)
{
#if CONFIG_BLUETOOTH_SNOOP_LOG
    if (pthread_mutex_init(&snoop_lock, NULL) < 0)
        return BT_STATUS_FAIL;

    if (btsnoop_create_new_file() < 0) {
        syslog(LOG_ERR, "%s fail", __func__);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
#else
    syslog(LOG_WARNING, "%s\n", "CONFIG_BLUETOOTH_SNOOP_LOG not set");
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

void btsnoop_log_capture(uint8_t is_recieve, uint8_t* hci_pkt, uint32_t hci_pkt_size)
{
#if CONFIG_BLUETOOTH_SNOOP_LOG
    pthread_mutex_lock(&snoop_lock);
    btsnoop_write_log(is_recieve, hci_pkt, hci_pkt_size);
    pthread_mutex_unlock(&snoop_lock);
#endif
}

void btsnoop_log_close(void)
{
#if CONFIG_BLUETOOTH_SNOOP_LOG
    btsnoop_close_file();
    pthread_mutex_destroy(&snoop_lock);
#endif
}

void btsnoop_log_filter_add_l2cid(uint16_t cid)
{
}

void btsnoop_log_filter_remove_l2cid(uint16_t cid)
{
}

void btsnoop_log_filter_add_packet_type(uint16_t packet_type)
{
}

void btsnoop_log_filter_remove_packet_type(uint16_t packet_type)
{
}
