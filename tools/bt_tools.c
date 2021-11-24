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
#define LOG_TAG "bttools"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system/readline.h>

#include "btm_gap.h"
#include "btm_manager.h"

#include "bt_tools.h"
#include "utils/log.h"

static void usage(void);
static int usage_cmd(void* handle, int argc, char** argv);
static int enable_cmd(void* handle, int argc, char** argv);
static int hfp_cmd(void* handle, int argc, char** argv);
static int quit_cmd(void* handle, int argc, char** argv);

static btm_gap_interface_t* gap_test_interface = NULL;
static btm_interface_t* manager;
static void* manager_handle = NULL;
static void* gap_hanlde = NULL;

static struct option main_options[] = {
    { "help", 0, 0, 'h' },
    { "version", 0, 0, 'v' },
    { 0, 0, 0, 0 }
};

static bt_command_t g_cmd_tables[] = {
    { "enable", enable_cmd, "enable stack" },
    { "disable", NULL, "disable stack" },
#ifdef CONFIG_BLUETOOTH_SPP
    { "spp", spp_command, "<SPP> Serial Port Profile" },
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    { "hfp", hfp_client_command, "<HFP> HandFree Profile --Client" },
#endif
    { "a2dpsrc", NULL, "<A2DP> Advanced Audio Distribution Profile --Source" },
    { "help", usage_cmd, "Usage for bttools" },
    { "quit", quit_cmd, "Quit" },
};

static void manager_init_status_changed_callback(bt_result_code status)
{
    BT_LOGD("%s", __func__);
}

static void manager_state_changed_callback(bt_manager_bt_state state)
{
    BT_LOGD("%s", __func__);
    char local_name[] = "BLUELET_NUTTX_Fzw";
    gap_test_interface->bt_set_local_name(gap_hanlde, local_name, sizeof(local_name));
    gap_test_interface->bt_set_local_device_class(gap_hanlde, BT_COD_SERVICE_RENDERING | BT_COD_SERVICE_AUDIO | BT_COD_SERVICE_TELEPHONY | BT_COD_AV_HEADSET);
    gap_test_interface->bt_set_local_io_capability(gap_hanlde, SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT);
    gap_test_interface->bt_set_scan_mode(gap_hanlde, SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
}

static void test_discovery_state_changed_callback(void* gap_handle, bt_discovery_state state)
{
    BT_LOGD("%s", __func__);
}

static void test_adapter_state_changed_callback(void* gap_handle, stack_state_t state)
{
    BT_LOGD("%s", __func__);
}

static void test_device_found_callback(void* gap_handle, bt_device_t* device)
{
    BT_LOGD("%s, device %02x%02x%02x%02x%02x%02x", __func__, device->addr[0], device->addr[1], device->addr[2], device->addr[3], device->addr[4], device->addr[5]);
}

const btm_gap_callbacks_t gap_test_callbacks = {
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .state_changed_cb = test_adapter_state_changed_callback,
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .device_found_callback_cb = test_device_found_callback,
};

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
    .init_status_changed_callback_cb = manager_init_status_changed_callback,
};

static int enable_cmd(void* handle, int argc, char** argv)
{
    manager->enable(handle);

    return 0;
}

static int hfp_cmd(void* handle, int argc, char** argv)
{
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]:%s\n", i, argv[i]);
    }
    return 0;
}

static int usage_cmd(void* handle, int argc, char** argv)
{
    usage();

    return 0;
}

static int quit_cmd(void* handle, int argc, char** argv)
{
    //manager->cleanup(handle);

    return 0;
}

static void usage(void)
{
    printf("Usage:\n"
           "\tbttool [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        printf("\t%-4s\t%s\n", g_cmd_tables[i].cmd, g_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tbttool <command> --help\n");
}

static void show_version(void)
{
    printf("Version :0.1.1\n");
}

static int execute_command(void* handle, int argc, char* argv[])
{
    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        if (strncmp(g_cmd_tables[i].cmd, argv[0], strlen(argv[0])) == 0) {
            if (g_cmd_tables[i].func) {
                g_cmd_tables[i].func(handle, argc, &argv[0]);
                if (g_cmd_tables[i].func == quit_cmd)
                    return -2;
                return 0;
            }
        }
    }

    printf("UnKnow command %s\n", argv[0]);
    usage();

    return -1;
}

int main(int argc, char** argv)
{
    int opt;
    int _argc = 0;
    char* _argv[10];
    char* buffer;
    char* saveptr;
    int ret, len;

    while ((opt = getopt_long(argc, argv, "h-v", main_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            exit(0);
        case 'v':
            show_version();
            exit(0);
        default:
            break;
        }
    }

    //btm_manager init
    manager = get_bt_manager_interface();
    manager->init(&manager_handle, &mgt_cb);
    gap_test_interface = get_gap_instance();
    gap_test_interface->gap_register_callbacks(manager_handle, &gap_hanlde, &gap_test_callbacks);

    buffer = malloc(CONFIG_NSH_LINELEN);
    if (!buffer)
        return -ENOMEM;

    while (1) {
        printf("bttool> ");
        fflush(stdout);

        len = readline(buffer, CONFIG_NSH_LINELEN, stdin, stdout);
        buffer[len] = '\0';
        if (len < 0)
            continue;

        if (buffer[0] == '!') {
#ifdef CONFIG_SYSTEM_SYSTEM
            system(buffer + 1);
#endif
            continue;
        }

        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        saveptr = NULL;
        char* tmpstr = buffer;

        while ((tmpstr = strtok_r(tmpstr, " ", &saveptr)) != NULL) {
            _argv[_argc] = tmpstr;
            _argc++;
            tmpstr = NULL;
        }

        if (_argc > 0) {
            ret = execute_command(manager_handle, _argc, _argv);
            memset(_argv, 0, sizeof(_argv));
            _argc = 0;
            if (ret == -2)
                break;
        }
    }

    free(buffer);
    //exit(1);
    return 0;
}