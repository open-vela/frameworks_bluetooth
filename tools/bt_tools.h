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
#ifndef __BT_TOOLS_H__
#define __BT_TOOLS_H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define ARRAY_SIZE(x)        (sizeof(x) / sizeof((x)[0]))
#define CMD_OK               (0)
#define CMD_INVALID_PARAM    (-1)
#define CMD_INVALID_OPT      (-4)
#define CMD_INVALID_ADDR     (-5)
#define CMD_PARAM_NOT_ENOUGH (-6)
#define CMD_UNKNOWN          (-7)
#define CMD_USAGE_FAULT      (-8)
#define CMD_ERROR            (-9)

#define BTTOOL_PRINT_USE_SYSLOG 1

#define LOG_TAG "[bttool]"

#if BTTOOL_PRINT_USE_SYSLOG
/* use syslog */
#include <debug.h>

#define PRINT(fmt, args...) syslog(LOG_DEBUG, LOG_TAG" " fmt "\n", ##args)
#else
/* use printf */
#define PRINT(fmt, args...) printf(LOG_TAG" " fmt "\n", ##args)
#endif

#define PRINT_ADDR(fmt, addr, ...)                 \
    do {                                           \
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 }; \
        bt_addr_ba2str(addr, addr_str);            \
        PRINT(fmt, addr_str, ##__VA_ARGS__);       \
    } while (0);

/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef struct {
    char *cmd; /* command */
    int (*func)(void *handle, int argc, char **argv); /* command func */
    int opt; /* use option parameters */
    char *help; /* usage  */
} bt_command_t;

int execute_command_in_table(void *handle, bt_command_t *table, uint32_t table_size, int argc, char *argv[]);
int execute_command_in_table_offset(void *handle, bt_command_t *table, uint32_t table_size, int argc, char *argv[], uint8_t offset);

int adv_command_exec(void *handle, int argc, char *argv[]);

int scan_command_init(void *handle);
void scan_command_uninit(void *handle);
int scan_command_exec(void *handle, int argc, char *argv[]);

#endif /* __BT_TOOLS_H__ */
