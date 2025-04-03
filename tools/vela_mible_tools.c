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
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__NuttX__)
#include <system/readline.h>
#endif

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_tools.h"
#include "utils.h"

#include "vela_mible_manage.h"
#include "vela_mible_port.h"

typedef struct {
    uv_loop_t loop;
    uv_async_queue_t async;
    uv_thread_t thread;
    uv_sem_t ready;
    bool async_api;
} vela_mible_tool_t;

static void usage(void);
static int usage_cmd(void* handle, int argc, char** argv);
static int enable_cmd(void* handle, int argc, char** argv);
static int disable_cmd(void* handle, int argc, char** argv);
static int set_adapter_cmd(void* handle, int argc, char** argv);
static int get_adapter_cmd(void* handle, int argc, char** argv);
static int set_scanmode_cmd(void* handle, int argc, char** argv);
static int get_scanmode_cmd(void* handle, int argc, char** argv);
static int set_le_addr_cmd(void* handle, int argc, char** argv);
static int get_le_addr_cmd(void* handle, int argc, char** argv);
static int set_scan_parameters_cmd(void* handle, int argc, char** argv);
static int quit_cmd(void* handle, int argc, char** argv);

bt_instance_t* g_vela_mible_tool_ins = NULL;
static void* adapter_callback = NULL;
static bool g_cmd_had_inited = false;
bool g_auto_accept_pair = true;
bond_state_t g_bond_state = BOND_STATE_NONE;

static struct {
    int cmd_err_code;
    const char* cmd_err_code_desc;
} cmd_err_map[] = {
    { CMD_OK, "OK" },
    { CMD_INVALID_PARAM, "Invalid Parameter" },
    { CMD_INVALID_OPT, "Invalid Option" },
    { CMD_INVALID_ADDR, "Invalid Address" },
    { CMD_PARAM_NOT_ENOUGH, "Parameter Not Enough" },
    { CMD_UNKNOWN, "Unknown Command" },
    { CMD_USAGE_FAULT, "Command Usage Fault" },
    { CMD_ERROR, "API Return Error" },
};

static struct option main_options[] = {
    { "async", 0, 0, 'a' },
    { "help", 0, 0, 'h' },
    { "version", 0, 0, 'v' },
    { 0, 0, 0, 0 }
};

static bt_command_t g_cmd_tables[] = {
    { "enable", enable_cmd, 0, "enable stack" },
    { "disable", disable_cmd, 0, "disable stack" },
    { "set", set_adapter_cmd, 0, "set adapter information, input \'set help\' show usage" },
    { "get", get_adapter_cmd, 0, "get adapter information, input \'get help\' show usage" },
    { "adv", adv_command_exec, 0, "advertising cmd,   input \'adv\' show usage" },
    { "scan", scan_command_exec, 0, "scan cmd,          input \'scan\' show usage" },
    { "log", log_command, 0, "log control command" },
    { "help", usage_cmd, 0, "Usage for vela_mible_tools" },
    { "quit", quit_cmd, 0, "Quit" },
    { "q", quit_cmd, 0, "Quit" },
};

#define SET_IOCAP_USAGE "params: <io capability> (0:displayonly, 1:yes&no, 2:keyboardonly, 3:no-in/no-out 4:keyboard&display)"
#define SET_CLASS_USAGE "params: <local class of device>, range in 0x0-0xFFFFFC, the 2 least significant shall be 0b00, example: 0x00640404"
#define SET_SCANPARAMS_USAGE "set scan parameters, params: <mode>(0: INQUIRY, 1: PAGE), <type>(0: standard, 1: interlaced), <interval>(range in 18-4096), <window>(range in 17-4096)"

static bt_command_t g_set_cmd_tables[] = {
    { "scanmode", set_scanmode_cmd, 0, "params: <scan mode> (0:none, 1:connectable 2:connectable&discoverable)" },
    { "leaddr", set_le_addr_cmd, 0, "set ble adapter addr, params: <leaddr>" },
    { "scanparams", set_scan_parameters_cmd, 0, SET_SCANPARAMS_USAGE },
    { "help", NULL, 0, "show set help info" },
    //{ "", , "set " },
};

static bt_command_t g_get_cmd_tables[] = {
    { "scanmode", get_scanmode_cmd, 0, "get adapter scan mode" },
    { "leaddr", get_le_addr_cmd, 0, "get ble adapter addr" },
    { "help", NULL, 0, "show get help info" },
    //{ "", , "get " },
};

static void bt_tool_init(void* handle)
{
    scan_command_init(handle);
    g_cmd_had_inited = true;
}

static void bt_tool_uninit(void* handle)
{
    if (!g_cmd_had_inited)
        return;

    scan_command_uninit(handle);
    g_cmd_had_inited = false;
}

static const char* cmd_err_str(int err_code)
{
    for (int i = 0; i < ARRAY_SIZE(cmd_err_map); i++) {
        if (cmd_err_map[i].cmd_err_code == err_code)
            return cmd_err_map[i].cmd_err_code_desc;
    }

    return "Correct code ?";
}

static int enable_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_enable(handle);
    return CMD_OK;
}

static int disable_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_disable(handle);
    return CMD_OK;
}

static void set_usage(void)
{
    printf("Usage:\n"
           "\tset [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_set_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_set_cmd_tables[i].cmd, g_set_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tset help\n");
}

static void get_usage(void)
{
    printf("Usage:\n"
           "\tget [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_get_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_get_cmd_tables[i].cmd, g_get_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tget help\n");
}

static int set_adapter_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        set_usage();
        return CMD_PARAM_NOT_ENOUGH;
    }

    int ret = execute_command_in_table(handle, g_set_cmd_tables, ARRAY_SIZE(g_set_cmd_tables), argc, argv);
    if (ret != CMD_OK)
        set_usage();

    return ret;
}

static int get_adapter_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        get_usage();
        return CMD_PARAM_NOT_ENOUGH;
    }

    int ret = execute_command_in_table(handle, g_get_cmd_tables, ARRAY_SIZE(g_get_cmd_tables), argc, argv);
    if (ret != CMD_OK)
        get_usage();

    return ret;
}

static int set_scanmode_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int scanmode = atoi(argv[0]);
    if (scanmode > BT_BR_SCAN_MODE_CONNECTABLE_DISCOVERABLE)
        return CMD_INVALID_PARAM;

    if (bt_adapter_set_scan_mode(handle, scanmode, 1) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Scan Mode:%d set success", scanmode);
    return CMD_OK;
}

static int get_scanmode_cmd(void* handle, int argc, char** argv)
{
    PRINT("Scan Mode:%d", bt_adapter_get_scan_mode(handle));
    return CMD_OK;
}

static int set_le_addr_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    bt_adapter_set_le_address(handle, &addr);

    return CMD_OK;
}

static int get_le_addr_cmd(void* handle, int argc, char** argv)
{
    bt_address_t addr;
    ble_addr_type_t type;

    bt_adapter_get_le_address(handle, &addr, &type);
    PRINT_ADDR("LE Address:%s, type:%d", &addr, type);

    return CMD_OK;
}

static int set_scan_parameters_cmd(void* handle, int argc, char** argv)
{
    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    int is_page = atoi(argv[0]);
    if (is_page != 0 && is_page != 1)
        return CMD_INVALID_PARAM;

    int type = atoi(argv[1]);
    if (type != 0 && type != 1)
        return CMD_INVALID_PARAM;

    int interval = atoi(argv[2]);
    if (interval < 0x12 || interval > 0x1000)
        return CMD_INVALID_PARAM;

    int window = atoi(argv[3]);
    if (window < 0x11 || window > 0x1000)
        return CMD_INVALID_PARAM;

    if (!is_page)
        bt_adapter_set_inquiry_scan_parameters(handle, type, interval, window);
    else
        bt_adapter_set_page_scan_parameters(handle, type, interval, window);

    return CMD_OK;
}

static int usage_cmd(void* handle, int argc, char** argv)
{
    if (argc == 2 && !strcmp(argv[1], "me!!!"))
        return -2;

    usage();

    return CMD_OK;
}

static int quit_cmd(void* handle, int argc, char** argv)
{
    return -2;
}

static void usage(void)
{
    printf("Usage:\n"
           "\tvela_mible_tool [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_cmd_tables[i].cmd, g_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tvela_mible_tool <command> --help\n");
}

static void show_version(void)
{
    printf("Version :0.0.1");
}

static int execute_command(void* handle, int argc, char* argv[])
{
    int ret;

    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        if (strlen(g_cmd_tables[i].cmd) == strlen(argv[0]) && strncmp(g_cmd_tables[i].cmd, argv[0], strlen(argv[0])) == 0) {
            if (g_cmd_tables[i].func) {
                if (g_cmd_tables[i].opt)
                    ret = g_cmd_tables[i].func(handle, argc, &argv[0]);
                else
                    ret = g_cmd_tables[i].func(handle, argc - 1, &argv[1]);
                if (g_cmd_tables[i].func == quit_cmd)
                    return -2;
                return ret;
            }
        }
    }

    PRINT("UnKnow command %s", argv[0]);
    usage();

    return CMD_UNKNOWN;
}

static void on_adapter_state_changed_cb(void* cookie, bt_adapter_state_t state)
{
    PRINT("Context:%p, Adapter state changed: %d", cookie, state);
    if (state == BT_ADAPTER_STATE_ON) {
        char name[64 + 1];

        bt_tool_init(g_vela_mible_tool_ins);
        /* get name */
        bt_adapter_get_name(g_vela_mible_tool_ins, name, 64);
        /* get io cap */
        bt_io_capability_t cap = bt_adapter_get_io_capability(g_vela_mible_tool_ins);
        /* get class */
        uint32_t class = bt_adapter_get_device_class(g_vela_mible_tool_ins);
        /* get scan mode */
        bt_scan_mode_t mode = bt_adapter_get_scan_mode(g_vela_mible_tool_ins);
        /* enable key derivation */
        bt_adapter_le_enable_key_derivation(g_vela_mible_tool_ins, true, true);
        bt_adapter_set_page_scan_parameters(g_vela_mible_tool_ins, BT_BR_SCAN_TYPE_INTERLACED, 0x400, 0x24);
        PRINT("Adapter Name: %s, Cap: %d, Class: 0x%08" PRIX32 ", Mode:%d", name, cap, class, mode);
    } else if (state == BT_ADAPTER_STATE_TURNING_OFF) {
        /* code */
        bt_tool_uninit(g_vela_mible_tool_ins);
    } else if (state == BT_ADAPTER_STATE_OFF) {
        /* do something */
    }
}

static void on_scan_mode_changed_cb(void* cookie, bt_scan_mode_t mode)
{
    PRINT("Adapter new scan mode: %d", mode);
}

#define LINK_TYPE(trans_) (trans_ == BT_TRANSPORT_BREDR ? "BREDR" : "LE")

const static adapter_callbacks_t g_adapter_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed_cb,
    .on_scan_mode_changed = on_scan_mode_changed_cb,
};

int execute_command_in_table_offset(void* handle, bt_command_t* table, uint32_t table_size, int argc, char* argv[], uint8_t offset)
{
    int ret;
    bt_command_t* cmd = table;

    for (int i = 0; i < table_size; i++) {
        if (strlen(cmd->cmd) == strlen(argv[0]) && strncmp(cmd->cmd, argv[0], strlen(argv[0])) == 0) {
            if (cmd->func) {
                ret = cmd->func(handle, argc - offset, &argv[offset]);
                return ret;
            }
        }
        cmd++;
    }
    PRINT("Erroneous command %s", argv[0]);

    return CMD_UNKNOWN;
}

int execute_command_in_table(void* handle, bt_command_t* table, uint32_t table_size, int argc, char* argv[])
{
    return execute_command_in_table_offset(handle, table, table_size, argc, argv, 1);
}

static int vela_mible_tool_ins_init(vela_mible_tool_t* vela_mible_tool)
{
    pthread_setschedprio(pthread_self(), CONFIG_BLUETOOTH_SERVICE_LOOP_THREAD_PRIORITY);
    g_vela_mible_tool_ins = bluetooth_create_instance();
    if (g_vela_mible_tool_ins == NULL) {
        PRINT("create instance error\n");
        return -1;
    }

    adapter_callback = bt_adapter_register_callback(g_vela_mible_tool_ins, &g_adapter_cbs);
    if (bt_adapter_get_state(g_vela_mible_tool_ins) == BT_ADAPTER_STATE_ON)
        bt_tool_init(g_vela_mible_tool_ins);

    return 0;
}

static void vela_mible_tool_ins_uninit(vela_mible_tool_t* vela_mible_tool)
{
    bt_tool_uninit(g_vela_mible_tool_ins);
    bt_adapter_unregister_callback(g_vela_mible_tool_ins, adapter_callback);
    bluetooth_delete_instance(g_vela_mible_tool_ins);
    g_vela_mible_tool_ins = NULL;
    adapter_callback = NULL;
}

#ifdef CONFIG_LIBUV_EXTENSION
static void handle_close_cb(uv_handle_t* handle)
{
    uv_stop(uv_handle_get_loop(handle));
}

static void vela_mible_tool_execute_command_cb(uv_async_queue_t* handle, void* buffer)
{
    int ret;
    int _argc = 0;
    char* _argv[32];
    char* saveptr = NULL;
    char* tmpstr = buffer;
    vela_mible_tool_t* vela_mible_tool = handle->data;

    memset(_argv, 0, sizeof(_argv));

    // 1. split command
    while ((tmpstr = strtok_r(tmpstr, " ", &saveptr)) != NULL) {
        _argv[_argc] = tmpstr;
        _argc++;
        tmpstr = NULL;
    }

    // 2. execute command
    if (_argc > 0) {
        if (vela_mible_tool->async_api) {
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
            ret = execute_async_command(g_vela_mible_tool_ins, _argc, _argv);
#else
            ret = CMD_INVALID_OPT;
#endif
        } else
            ret = execute_command(g_vela_mible_tool_ins, _argc, _argv);
        if (ret != CMD_OK) {
            if (ret == -2) {
                if (vela_mible_tool->async_api) {
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
                    vela_mible_tool_async_ins_uninit(vela_mible_tool);
#endif
                } else
                    vela_mible_tool_ins_uninit(vela_mible_tool);
                uv_async_queue_close(handle, handle_close_cb);
            } else
                PRINT("cmd execute error: [%s]", cmd_err_str(ret));
        }
    }

    free(buffer);
}

static void vela_mible_tool_command_uvloop_run(vela_mible_tool_t* vela_mible_tool)
{
    int ret;

    /* This code is used to initialize the async queue. */
    ret = uv_async_queue_init(&vela_mible_tool->loop, &vela_mible_tool->async, vela_mible_tool_execute_command_cb);
    if (ret != 0) {
        PRINT("%s async error: %d", __func__, ret);
        uv_loop_close(&vela_mible_tool->loop);
        return;
    }

    vela_mible_tool->async.data = vela_mible_tool;
    uv_sem_post(&vela_mible_tool->ready);

    /* This code is used to start the event loop until there are no more events to process. */
    uv_run(&vela_mible_tool->loop, UV_RUN_DEFAULT);

    /* The assert() function is used to check the return value of uv_loop_close().
       If the return value is 0, it means that the loop is closed successfully,
       otherwise it means an error occurs.
    */
    assert(uv_loop_close(&vela_mible_tool->loop) == 0);
}

static void vela_mible_tool_thread(void* data)
{
    vela_mible_tool_t* vela_mible_tool = data;

    /* Initialize the event loop, the loop is available
       before the asynchronous instance is created.
    */
    uv_loop_init(&vela_mible_tool->loop);

    /* initialize synchronous or asynchronous instance.
       and register callbacks.
    */
    if (!vela_mible_tool->async_api) {
        vela_mible_tool_ins_init(vela_mible_tool);
    } else {
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
        vela_mible_tool_async_ins_init(vela_mible_tool);
#endif
    }

    /* This code is used to start the event loop until there are no more events to process. */
    vela_mible_tool_command_uvloop_run(vela_mible_tool);
}

static int vela_mible_tool_create_thread(vela_mible_tool_t* vela_mible_tool)
{
    int ret;

    ret = uv_sem_init(&vela_mible_tool->ready, 0);
    if (ret != 0) {
        PRINT("%s sem init error: %d", __func__, ret);
        return ret;
    }

    ret = uv_thread_create(&vela_mible_tool->thread, vela_mible_tool_thread, (void*)vela_mible_tool);
    if (ret != 0) {
        PRINT("loop thread create :%d", ret);
        return ret;
    }

    pthread_setname_np(vela_mible_tool->thread, "vela_mible_tool-cmd-exec");
    uv_sem_wait(&vela_mible_tool->ready);
    uv_sem_destroy(&vela_mible_tool->ready);

    return 0;
}

static void vela_mible_tool_quit(vela_mible_tool_t* vela_mible_tool)
{
    char* buffer = malloc(5);

    strcpy(buffer, "quit");
    uv_async_queue_send(&vela_mible_tool->async, buffer);
}

int main(int argc, char** argv)
{
    int opt;
    char* buffer = NULL;
    int ret;
    size_t len, size = 0;
    vela_mible_tool_t vela_mible_tool = { .async_api = false };

    while ((opt = getopt_long(argc, argv, "a-h-v-d", main_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
#ifdef CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
            vela_mible_tool.async_api = true;
            break;
#else
            PRINT("async not supported");
            return -1;
#endif
        case 'h':
            usage();
            exit(0);
        case 'v':
            show_version();
            exit(0);
            break;
        default:
            break;
        }
    }

    // Call the vela_mible_tool_create_thread function to create a new thread
    // If thread creation fails, the return value is non-zero
    ret = vela_mible_tool_create_thread(&vela_mible_tool);
    if (ret != 0)
        return ret;

    while (1) {
        printf("vela_mible_tool> ");
        fflush(stdout);

        len = getline(&buffer, &size, stdin);
        if (-1 == len) {
            vela_mible_tool_quit(&vela_mible_tool);
            break;
        }

        buffer[len] = '\0';
        if (buffer[0] == '!') {
#ifdef CONFIG_SYSTEM_SYSTEM
            system(buffer + 1);
#endif
            continue;
        }

        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "q") == 0) {
            uv_async_queue_send(&vela_mible_tool.async, buffer);
            break;
        }

        uv_async_queue_send(&vela_mible_tool.async, buffer);

        buffer = NULL;
    }

    uv_thread_join(&vela_mible_tool.thread);

    return 0;
}
#else /* CONFIG_LIBUV_EXTENSION */
int main(int argc, char** argv)
{
    int opt;
    int _argc = 0;
    char* _argv[32];
    char* buffer = NULL;
    char* saveptr;
    int ret;
    size_t len, size = 0;

    while ((opt = getopt_long(argc, argv, "h-v-d", main_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            exit(0);
        case 'v':
            show_version();
            exit(0);
            break;
        default:
            break;
        }
    }

    vela_mible_tool_ins_init(NULL);

    while (1) {
        printf("vela_mible_tool> ");
        fflush(stdout);

        memset(_argv, 0, sizeof(_argv));
        len = getline(&buffer, &size, stdin);
        buffer[len] = '\0';
        if (len < 0)
            goto quit;

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
            ret = execute_command(g_vela_mible_tool_ins, _argc, _argv);
            _argc = 0;
            if (ret != CMD_OK) {
                if (ret == -2)
                    break;
                PRINT("cmd execute error: [%s]", cmd_err_str(ret));
            }
        }
    }

quit:
    vela_mible_tool_ins_uninit(NULL);
    free(buffer);

    return 0;
}
#endif /* CONFIG_LIBUV_EXTENSION */