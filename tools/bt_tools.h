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
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "[bttool]"

#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt_debug.h"
#include "utils.h"

#include "uv.h"
#include "uv_async_queue.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define CMD_OK (0)
#define CMD_INVALID_PARAM (-1)
#define CMD_INVALID_OPT (-4)
#define CMD_INVALID_ADDR (-5)
#define CMD_PARAM_NOT_ENOUGH (-6)
#define CMD_UNKNOWN (-7)
#define CMD_USAGE_FAULT (-8)
#define CMD_ERROR (-9)

#define BTTOOL_PRINT_USE_SYSLOG 0

#if BTTOOL_PRINT_USE_SYSLOG
/* use syslog */
#include <debug.h>

#define PRINT(fmt, args...) syslog(LOG_DEBUG, LOG_TAG " " fmt "\n", ##args)
#else
/* use printf */
#define PRINT(fmt, args...) printf(LOG_TAG " " fmt "\n", ##args)
#endif

#define PRINT_ADDR(fmt, addr, ...)                 \
    do {                                           \
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 }; \
        bt_addr_ba2str(addr, addr_str);            \
        PRINT(fmt, addr_str, ##__VA_ARGS__);       \
    } while (0);

#ifndef CONFIG_NSH_LINELEN
#define CONFIG_NSH_LINELEN 80
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/
typedef struct {
    uv_loop_t loop;
    uv_async_queue_t async;
    uv_thread_t thread;
    uv_sem_t ready;
    bool async_api;
} bttool_t;

typedef struct {
    char* cmd; /* command */
    int (*func)(void* handle, int argc, char** argv); /* command func */
    int opt; /* use option parameters */
    char* help; /* usage  */
} bt_command_t;

int execute_async_command(void* handle, int argc, char* argv[]);
int bttool_async_ins_init(bttool_t* bttool);
void bttool_async_ins_uninit(bttool_t* bttool);

int execute_command_in_table(void* handle, bt_command_t* table, uint32_t table_size, int argc, char* argv[]);
int execute_command_in_table_offset(void* handle, bt_command_t* table, uint32_t table_size, int argc, char* argv[], uint8_t offset);

int log_command(void* handle, int argc, char* argv[]);
int adv_command_exec(void* handle, int argc, char* argv[]);

int scan_command_init(void* handle);
void scan_command_uninit(void* handle);
int scan_command_exec(void* handle, int argc, char* argv[]);

int a2dp_sink_commond_init(void* handle);
int a2dp_sink_commond_uninit(void* handle);
int a2dp_sink_command_exec(void* handle, int argc, char* argv[]);

int a2dp_src_commond_init(void* handle);
int a2dp_src_commond_uninit(void* handle);
int a2dp_src_command_exec(void* handle, int argc, char* argv[]);

int avrcp_control_commond_init(void* handle);
int avrcp_control_commond_uninit(void* handle);
int avrcp_control_command_exec(void* handle, int argc, char* argv[]);

int hfp_hf_commond_init(void* handle);
int hfp_hf_commond_uninit(void* handle);
int hfp_hf_command_exec(void* handle, int argc, char* argv[]);

int hfp_ag_commond_init(void* handle);
int hfp_ag_commond_uninit(void* handle);
int hfp_ag_command_exec(void* handle, int argc, char* argv[]);

int spp_command_init(void* handle);
void spp_command_uninit(void* handle);
int spp_command_exec(void* handle, int argc, char* argv[]);

int hidd_command_init(void* handle);
void hidd_command_uninit(void* handle);
int hidd_command_exec(void* handle, int argc, char* argv[]);

int pan_command_init(void* handle);
void pan_command_uninit(void* handle);
int pan_command_exec(void* handle, int argc, char* argv[]);

int gattc_command_init(void* handle);
int gattc_command_uninit(void* handle);
int gattc_command_exec(void* handle, int argc, char* argv[]);

int gatts_command_init(void* handle);
int gatts_command_uninit(void* handle);
int gatts_command_exec(void* handle, int argc, char* argv[]);

int leas_command_init(void* handle);
void leas_command_uninit(void* handle);
int leas_command_exec(void* handle, int argc, char* argv[]);

int lea_mcp_commond_init(void* handle);
void lea_mcp_commond_uninit(void* handle);
int lea_mcp_command_exec(void* handle, int argc, char* argv[]);

int lea_ccp_command_init(void* handle);
void lea_ccp_command_uninit(void* handle);
int lea_ccp_command_exec(void* handle, int argc, char* argv[]);

int lea_vmics_command_init(void* handle);
void lea_vmics_command_uninit(void* handle);
int vmics_command_exec(void* handle, int argc, char* argv[]);

int leac_command_init(void* handle);
void leac_command_uninit(void* handle);
int leac_command_exec(void* handle, int argc, char* argv[]);

int lea_mcs_commond_init(void* handle);
void lea_mcs_commond_uninit(void* handle);
int lea_mcs_command_exec(void* handle, int argc, char* argv[]);

int lea_tbs_command_init(void* handle);
void lea_tbs_command_uninit(void* handle);
int lea_tbs_command_exec(void* handle, int argc, char* argv[]);

int lea_vmicp_command_init(void* handle);
void lea_vmicp_command_uninit(void* handle);
int vmicp_command_exec(void* handle, int argc, char* argv[]);

#endif /* __BT_TOOLS_H__ */
