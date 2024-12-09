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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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
#include "btsnoop_writer.h"

#define SNOOP_FILE_NAME_PREFIX "/snoop_"
#define SNOOP_FILE_NAME_PREFIX_LEN 7
#define SNOOP_FILE_NAME_DATE_LEN 80
#define SNOOP_FILE_NAME_SUFFIX "%s_%" PRIu32 ".log"
#define SNOOP_FILE_NAME_SUFFIX_LEN (20 + SNOOP_FILE_NAME_DATE_LEN)
#define SNOOP_FILE_NAME SNOOP_FILE_NAME_PREFIX SNOOP_FILE_NAME_SUFFIX
#define SNOOP_FILE_NAME_LEN (SNOOP_FILE_NAME_PREFIX_LEN + SNOOP_FILE_NAME_SUFFIX_LEN)
#define SNOOP_FILE_FULL_NAME_MAX_LEN (SNOOP_FILE_NAME_LEN + SNOOP_PATH_MAX_LEN)
#define SNOOP_FILE_TYPE 1002

#define write_snoop_file(buf, buf_size)                                               \
    do {                                                                              \
        ret = write(g_using_file.snoop_fd, buf, buf_size);                            \
        if (ret < 0)                                                                  \
            syslog(LOG_ERR, "snoop log header write ret:%d, error:%d\n", ret, errno); \
        else                                                                          \
            g_using_file.size += ret;                                                 \
                                                                                      \
    } while (0)

typedef struct {
    int snoop_fd;
    size_t size;
} btsnoop_file_t;

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
};

static time_t time_base;
static uint32_t ms_base;
static btsnoop_file_t g_using_file = { 0 };
static char g_snoop_file_path[SNOOP_PATH_MAX_LEN + 1];

static void close_snoop_file(void)
{
    if (g_using_file.snoop_fd > 0) {
        fsync(g_using_file.snoop_fd);
        close(g_using_file.snoop_fd);
        g_using_file.snoop_fd = 0;
        g_using_file.size = 0;
    }
}
static uint32_t get_current_time_ms(void)
{
    return (uint32_t)(get_os_timestamp_us() / 1000);
}

static unsigned long byteswap_ulong(unsigned long val)
{
    unsigned char* byte_val = (unsigned char*)&val;
    return ((unsigned long)byte_val[3] + ((unsigned long)byte_val[2] << 8) + ((unsigned long)byte_val[1] << 16) + ((unsigned long)byte_val[0] << 24));
}

static int get_latest_file_and_clean_others(char* out_latest_file, bool clean_files)
{
    DIR* dir;
    struct dirent* entry;
    struct stat file_stat;
    time_t latest_time = -1;
    char* full_path;
    char* latest_file;

    full_path = zalloc(SNOOP_FILE_FULL_NAME_MAX_LEN + 1);
    if (full_path == NULL) {
        return BT_STATUS_FAIL;
    }

    latest_file = zalloc(SNOOP_FILE_FULL_NAME_MAX_LEN + 1);
    if (latest_file == NULL) {
        free(full_path);
        return BT_STATUS_FAIL;
    }

    dir = opendir(g_snoop_file_path);
    if (dir == NULL) {
        syslog(LOG_ERR, "snoop folder open fail:%d", errno);
        free(latest_file);
        free(full_path);
        return BT_STATUS_FAIL;
    }

    while ((entry = readdir(dir)) != NULL) {
        snprintf(full_path, SNOOP_FILE_FULL_NAME_MAX_LEN, "%s/%s", g_snoop_file_path, entry->d_name);
        if (strncmp(entry->d_name, SNOOP_FILE_NAME_PREFIX, strlen(SNOOP_FILE_NAME_PREFIX)) != 0) {
            continue;
        }

        if (stat(full_path, &file_stat) != 0) {
            syslog(LOG_ERR, "get snoop file stat fail:%d", errno);
            continue;
        }

        if (!S_ISREG(file_stat.st_mode)) {
            continue;
        }

        if (clean_files && file_stat.st_mtime > latest_time) {
            remove(latest_file);
        } else if (clean_files && latest_time != -1) {
            remove(full_path);
        }

        if (latest_time == -1 || file_stat.st_mtime > latest_time) {
            latest_time = file_stat.st_mtime;
            strlcpy(latest_file, full_path, SNOOP_FILE_FULL_NAME_MAX_LEN);
        }
    }

    if (NULL != out_latest_file) {
        strlcpy(out_latest_file, latest_file, SNOOP_FILE_FULL_NAME_MAX_LEN);
    }

    closedir(dir);

    free(latest_file);
    free(full_path);
    return BT_STATUS_SUCCESS;
}

int btsnoop_create_new_file(void)
{
    struct btsnoop_file_hdr hdr;
    time_t rawtime;
    struct tm* info;
    char ts_str[SNOOP_FILE_NAME_DATE_LEN + 1];
    int ret;
    char* full_file_name;

    close_snoop_file();

    if (-1 == mkdir(g_snoop_file_path, 0777) && errno != EEXIST) {
        syslog(LOG_ERR, "snoop folder create fail:%d", errno);
        return -errno;
    }

    time_base = time(NULL);
    ms_base = get_current_time_ms();

    time(&rawtime);
    info = localtime(&rawtime);
    if (info == NULL) {
        return -1;
    }

    snprintf(ts_str, sizeof(ts_str), "%d%02d%02d_%02d%02d%02d",
        info->tm_year + 1900,
        info->tm_mon + 1,
        info->tm_mday,
        info->tm_hour,
        info->tm_min,
        info->tm_sec);

    full_file_name = malloc(SNOOP_FILE_FULL_NAME_MAX_LEN + 1);
    snprintf(full_file_name, SNOOP_FILE_FULL_NAME_MAX_LEN, "%s" SNOOP_FILE_NAME, g_snoop_file_path, ts_str, ms_base);
    ret = open(full_file_name, O_RDWR | O_CREAT | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    free(full_file_name);

    if (ret < 0) {
        g_using_file.snoop_fd = -1;
        return ret;
    }

    g_using_file.snoop_fd = ret;
    g_using_file.size = 0;
    syslog(LOG_ERR, "create fd:%d", ret);

    memcpy(hdr.id, "btsnoop", sizeof(hdr.id));
    hdr.version = byteswap_ulong(1);
    hdr.type = byteswap_ulong(SNOOP_FILE_TYPE);

    write_snoop_file(&hdr, sizeof(hdr));
    return ret;
}

int open_snoop_file(char* latest_file)
{
    int fd;
    size_t file_size;

    fd = open(latest_file, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

    if (fd < 0) {
        syslog(LOG_ERR, "open snoop file fail:%d", errno);
        return fd;
    }

    file_size = lseek(fd, 0, SEEK_END);

    g_using_file.snoop_fd = fd;
    g_using_file.size = file_size;

    return BT_STATUS_SUCCESS;
}

void set_snoop_file_path(char* path)
{
    strlcpy(g_snoop_file_path, path, SNOOP_PATH_MAX_LEN);
}

int writer_init()
{
    DIR* dir;
    char latest_file[SNOOP_FILE_FULL_NAME_MAX_LEN + 1] = "";

    dir = opendir(g_snoop_file_path);
    if (dir == NULL) {
        closedir(dir);
        return btsnoop_create_new_file();
    }
    closedir(dir);

    get_latest_file_and_clean_others(latest_file, false);

    if (latest_file[0] == '\0') {
        return btsnoop_create_new_file();
    }

    return open_snoop_file(latest_file);
}

void writer_uninit()
{
    close_snoop_file();
}

int writer_write_log(uint8_t is_recieve, uint8_t* p, uint32_t len)
{
    struct btsnoop_pkt_hdr pkt;
    uint32_t ms;
    int ret;

    if (g_using_file.snoop_fd < 0)
        return BT_STATUS_FAIL;

    if (g_using_file.size + sizeof(pkt) + len > CONFIG_MAX_SNOOP_FILE_SIZE) {
        get_latest_file_and_clean_others(NULL, true);
        ret = btsnoop_create_new_file();
        if (ret < 0)
            return ret;
    }

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

    write_snoop_file(&pkt, sizeof(pkt));
    write_snoop_file(p, len);

    fsync(g_using_file.snoop_fd);

    return BT_STATUS_SUCCESS;
}
