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
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include "advertiser_data.h"
#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_le_advertiser.h"
#include "bt_tools.h"
#include "service_loop.h"
#include "uv_ext.h"

#include "storage.h"
#include "storage_update.h"
#include "storage_version_4.h"
#include "storage_version_5.h"

#define BT_STORAGE_TEST_VERSION "v_test"

static int storage_add_cmd(void* handle, int argc, char* argv[]);
static int storage_clear_cmd(void* handle, int argc, char* argv[]);
static int storage_set_cmd(void* handle, int argc, char* argv[]);
static int storage_delete_cmd(void* handle, int argc, char* argv[]);
static int storage_update_cmd(void* handle, int argc, char* argv[]);

static char* bt_storage_update_version_str[BT_STORAGE_VERSION_MAX] = {
    "v4_0_0",
    "v5_0_0",
    "v5_0_1",
    "v5_0_2",
};

static struct option set_options[] = {
    { "default", no_argument, 0, 'D' },
    { "version", required_argument, 0, 'v' },
    { "scanmode", required_argument, 0, 's' },
    { "iocap", required_argument, 0, 'i' },
    { "name", required_argument, 0, 'n' },
    { "class", required_argument, 0, 'c' },
    { "bondable", required_argument, 0, 'b' },
    { "test", no_argument, 0, 'T' },
    { 0, 0, 0, 0 }
};

static struct option delete_options[] = {
    { "scanmode", required_argument, 0, 's' },
    { "iocap", required_argument, 0, 'i' },
    { "name", required_argument, 0, 'n' },
    { "class", required_argument, 0, 'c' },
    { "bondable", required_argument, 0, 'b' },
    { 0, 0, 0, 0 }
};

#define SET_IOCAP_USAGE "set io capability (0:displayonly, 1:yes&no, 2:keyboardonly, 3:no-in/no-out 4:keyboard&display)"
#define SET_CLASS_USAGE "set local class of device, range in 0x0-0xFFFFFC, the 2 least significant shall be 0b00, example: 0x00640404"

static bt_command_t g_storage_tables[] = {
    { "add", storage_add_cmd, 0, "\"add storage information :<version_idx><storage_info(1:btbond,2:blebond,3:whitelist(accept list))><num>\"" },
    { "clear", storage_clear_cmd, 0, "clear storage information  \n" },
    { "set", storage_set_cmd, 1, "set adapter information(only for V5_0_2 version and above),"
                                 "\t  -D or --default, set adapter default infomation\n"
                                 "\t  -v or --version, set version(0:v4_0_0, 1:v5_0_0, 2:v5_0_1, 3:v5_0_2)\n"
                                 "\t  -s or --scanmode, set scan mode (0:none, 1:connectable 2:connectable&discoverable)\n"
                                 "\t  -i or --iocap, " SET_IOCAP_USAGE "\n"
                                 "\t  -n or --name, set local name, example \"vela-bt\"\n"
                                 "\t  -c or --class, " SET_CLASS_USAGE " \n"
                                 "\t  -b or --bonable, now only can set bondable(1) \n" },
    { "delete", storage_delete_cmd, 1, "delete adapter information,"
                                       "\t  -s or --scanmode, delete scan mode\n"
                                       "\t  -i or --iocap, delete iocap\n"
                                       "\t  -n or --name, delete name\n"
                                       "\t  -c or --class, delete class \n"
                                       "\t  -b or --bonable, delete bondable \n" },
    { "update", storage_update_cmd, 0, "storage update version. \n" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_storage_tables); i++) {
        printf("\t%-4s\t%s\n", g_storage_tables[i].cmd, g_storage_tables[i].help);
    }
}

static int kvdb_param_check(int input_version, int storage_version)
{
    if (input_version == -1) {
        PRINT("please input version first!!");
        return CMD_INVALID_PARAM;
    }

    if (input_version < BT_STORAGE_VERSION_5_0_2) {
        PRINT("only support v5.0.2 version(%d) +, cur = %d\n", BT_STORAGE_VERSION_5_0_2, storage_version);
        return CMD_INVALID_PARAM;
    }

    if (input_version != storage_version) {
        PRINT("version mismatch!!, cur = %d, input = %d\n", storage_version, input_version);
        return CMD_INVALID_PARAM;
    }

    return CMD_OK;
}

static int storage_set_cmd(void* handle, int argc, char* argv[])
{
    int cur_version, opt, adapter_size;
    int input_version = -1, ret = CMD_OK;
    void* adapter;
    char* version_str;
    char name[BT_LOC_NAME_MAX_LEN + 1];
    bool fallback_test_mode = false;

    if (bt_storage_unqlite_init() != 0)
        return CMD_ERROR;

    cur_version = bt_storage_get_version();
    optind = 0;

    while ((opt = getopt_long(argc, argv, "Dv:s:i:n:c:b:", set_options,
                NULL))
        != -1) {
        switch (opt) {
        case 'D': {
            if (input_version == -1) {
                PRINT("please input version first!!");
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            snprintf(name, BT_LOC_NAME_MAX_LEN + 1, "%s-%s", "adapter_name", bt_storage_update_version_str[input_version]);
            if (input_version < BT_STORAGE_VERSION_5_0_2) {
                adapter_size = bt_storage_update_get_item_len(input_version, BT_STORAGE_UPDATE_ADAPTER_INFO);
                adapter = malloc(adapter_size);
                memset(adapter, 1, adapter_size);
                memcpy(adapter, name, strlen(name) + 1);
                ret = bt_storage_save_item_unqlite(adapter, 1, input_version, BT_STORAGE_UPDATE_ADAPTER_INFO);
                free(adapter);
                if (ret < 0) {
                    PRINT("save adapter info failed!!");
                    goto err;
                }
            } else if (input_version >= BT_STORAGE_VERSION_5_0_2) {
                property_set_binary(BT_KVDB_ADAPTERINFO_NAME, name, strlen(name) + 1, false);
                property_set_int32(BT_KVDB_ADAPTERINFO_COD, DEFAULT_DEVICE_OF_CLASS);
                property_set_int32(BT_KVDB_ADAPTERINFO_IOCAP, DEFAULT_IO_CAPABILITY);
                property_set_int32(BT_KVDB_ADAPTERINFO_SCAN, DEFAULT_SCAN_MODE);
                property_set_int32(BT_KVDB_ADAPTERINFO_BOND, DEFAULT_BONDABLE_MODE);
            }
        } break;
        case 'v': {
            input_version = atoi(optarg);

            if (cur_version != -1 && input_version != cur_version) {
                PRINT("error version!!");
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            version_str = bt_storage_update_version_str[input_version];
            if (input_version >= BT_STORAGE_VERSION_5_0_2)
                property_set_binary(BT_KVDB_VERSION_KEY, version_str, strlen(version_str) + 1, false);
            PRINT("version: %s", version_str);
        } break;
        case 's': {
            int scanmode = atoi(optarg);

            ret = kvdb_param_check(input_version, cur_version);
            if (ret != CMD_OK)
                goto err;

            if (scanmode < BT_BR_SCAN_MODE_NONE || scanmode > BT_BR_SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            property_set_int32(BT_KVDB_ADAPTERINFO_SCAN, scanmode);
            PRINT("Scan Mode:%d set success", scanmode);
        } break;
        case 'i': {
            int iocap = atoi(optarg);

            ret = kvdb_param_check(input_version, cur_version);
            if (ret != CMD_OK)
                goto err;

            if (iocap < BT_IO_CAPABILITY_DISPLAYONLY || iocap > BT_IO_CAPABILITY_KEYBOARDDISPLAY) {
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            property_set_int32(BT_KVDB_ADAPTERINFO_IOCAP, iocap);
            PRINT("IO Capability:%d set success", iocap);
        } break;
        case 'n': {
            ret = kvdb_param_check(input_version, cur_version);
            if (ret != CMD_OK)
                goto err;

            if (strlen(optarg) > BT_LOC_NAME_MAX_LEN) {
                PRINT("name length to long");
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            property_set_binary(BT_KVDB_ADAPTERINFO_NAME, optarg, strlen(optarg) + 1, false);
            PRINT("Local Name:%s set success", optarg);
        } break;
        case 'c': {
            uint32_t cod = atoi(optarg);

            ret = kvdb_param_check(input_version, cur_version);
            if (ret != CMD_OK)
                goto err;

            if (cod > 0xFFFFFF || cod & 0x3) {
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            property_set_int32(BT_KVDB_ADAPTERINFO_COD, cod);
            PRINT("Local class of device:0x%08" PRIx32 " set success", cod);
        } break;
        case 'b': {
            int bondable = atoi(optarg);

            ret = kvdb_param_check(input_version, cur_version);
            if (ret != CMD_OK)
                goto err;

            if (bondable != 1) {
                PRINT("only bondable only input <1>");
                ret = CMD_INVALID_PARAM;
                goto err;
            }

            property_set_int32(BT_KVDB_ADAPTERINFO_BOND, bondable);
            PRINT("bondable: %d set success", bondable);
        } break;
        default:
            PRINT("%s, default opt:%c, arg:%s", __func__, opt, optarg);
            break;
        }
    }

    if (fallback_test_mode) {
        property_set_binary(BT_KVDB_VERSION_KEY, BT_STORAGE_TEST_VERSION, strlen(BT_STORAGE_TEST_VERSION) + 1, false);
    }

    property_commit();

err:
    bt_storage_unqlite_cleanup();
    return ret;
}

static int storage_delete_cmd(void* handle, int argc, char* argv[])
{
    int cur_version, opt;
    int ret = CMD_OK;

    if (bt_storage_unqlite_init() != 0)
        return CMD_ERROR;

    cur_version = bt_storage_get_version();
    optind = 0;

    if (cur_version < BT_STORAGE_VERSION_5_0_2) {
        PRINT("only support v5.0.2 version[%d] +, cur = %d\n", BT_STORAGE_VERSION_5_0_2, cur_version);
        return CMD_INVALID_PARAM;
    }

    while ((opt = getopt_long(argc, argv, "+sincb", delete_options,
                NULL))
        != -1) {
        switch (opt) {
        case 's': {
            if (property_delete(BT_KVDB_ADAPTERINFO_SCAN)) {
                PRINT("Scan Mode delete failed");
                ret = CMD_ERROR;
                goto err;
            }

            PRINT("Scan Mode delete success");
        } break;
        case 'i': {
            if (property_delete(BT_KVDB_ADAPTERINFO_IOCAP)) {
                PRINT("iocap delete failed");
                ret = CMD_ERROR;
                goto err;
            }

            PRINT("iocap delete success");
        } break;
        case 'n': {
            if (property_delete(BT_KVDB_ADAPTERINFO_NAME)) {
                PRINT("adapter name delete failed");
                ret = CMD_ERROR;
                goto err;
            }

            PRINT("adapter name delete success");
        } break;
        case 'c': {
            if (property_delete(BT_KVDB_ADAPTERINFO_COD)) {
                PRINT("CoD delete failed");
                ret = CMD_ERROR;
                goto err;
            }

            PRINT("CoD delete success");
        } break;
        case 'b': {
            if (property_delete(BT_KVDB_ADAPTERINFO_BOND)) {
                PRINT("bondable delete failed");
                ret = CMD_ERROR;
                goto err;
            }

            PRINT("bondable delete success");
        } break;
        default:
            PRINT("%s, default opt:%c, arg:%s", __func__, opt, optarg);
            break;
        }
    }

    property_commit();

err:
    bt_storage_unqlite_cleanup();
    return ret;
}

static uint8_t* generate_storage_item(int version, int item, int num)
{
    int item_size, i, offset;
    uint8_t *data, *tmp_data;

    item_size = bt_storage_update_get_item_len(version, item);
    data = (uint8_t*)malloc(item_size * num);
    if (!data) {
        PRINT("Malloc failed");
        return NULL;
    }
    tmp_data = data;

    for (i = 0; i < num; i++) {
        memset(tmp_data, i, item_size);
        if (item != BT_STORAGE_UPDATE_BTBOND_INFO) {
            tmp_data += item_size;
            continue;
        }

        if (version < BT_STORAGE_VERSION_5_0_2) {
            offset = offsetof(remote_device_properties_v4_0_0_t, name);
            snprintf((char*)tmp_data + offset, 64, "%s-%d", "NAME-TEST", i);
            offset += version < BT_STORAGE_VERSION_5_0_0 ? 64 : 65;
            snprintf((char*)tmp_data + offset, 64, "%s-%d", "ALIAS-TEST", i);
        } else if (version >= BT_STORAGE_VERSION_5_0_2) {
            offset = offsetof(remote_device_properties_v5_0_2_t, name);
            snprintf((char*)tmp_data + offset, 64, "%s-%d", "NAME-TEST", i);
            offset += 65;
            snprintf((char*)tmp_data + offset, 64, "%s-%d", "ALIAS-TEST", i);
        }

        tmp_data += item_size;
    }

    return data;
}

static int storage_add_cmd(void* handle, int argc, char* argv[])
{
    int input_version, cur_version, item, num, ret;
    uint8_t* data = NULL;

    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_storage_unqlite_init() != 0)
        return CMD_ERROR;

    cur_version = bt_storage_get_version();
    input_version = atoi(argv[1]);
    if (cur_version >= 0 && cur_version != input_version) {
        PRINT("Invalid version:%d, please input current version: %d", input_version, cur_version);
        ret = CMD_INVALID_PARAM;
        goto err;
    }

    item = atoi(argv[2]);
    if (item < BT_STORAGE_UPDATE_BTBOND_INFO || item > BT_STORAGE_UPDATE_ITEM_MAX) {
        PRINT("Invalid item:%d, please input 1 ~ %d", item, BT_STORAGE_UPDATE_ITEM_MAX - 1);
        ret = CMD_INVALID_PARAM;
        goto err;
    }

    num = atoi(argv[3]);
    if (num < 1 || num > 15) {
        PRINT("Invalid num:%d, please input 1 ~ 15", num);
        ret = CMD_INVALID_PARAM;
        goto err;
    }

    data = generate_storage_item(input_version, item, num);
    if (!data) {
        PRINT("Generate storage item failed");
        ret = CMD_INVALID_PARAM;
        goto err;
    }

    if (input_version < BT_STORAGE_VERSION_5_0_2) {
        ret = bt_storage_save_item_unqlite(data, num, input_version, item);
    } else if (input_version >= BT_STORAGE_VERSION_5_0_2) {
        ret = bt_storage_save_item_kvdb(data, num, input_version, item);
    }

err:
    if (data)
        free(data);

    bt_storage_unqlite_cleanup();

    return ret;
}

static int storage_clear_cmd(void* handle, int argc, char* argv[])
{
    int ret;

    ret = bt_storage_remove();
    if (ret < 0) {
        return CMD_ERROR;
    }

    return CMD_OK;
}

static int storage_update_cmd(void* handle, int argc, char* argv[])
{
#ifdef CONFIG_SYSTEM_SYSTEM
    system("bt_storage_update");
#else
    PRINT("storage udpate cmd not support");
    return CMD_ERROR;
#endif

    return CMD_OK;
}

/* init for unqlite storage version */
int storage_command_init(void* handle)
{
    return 0;
}

void storage_command_uninit(void* handle)
{
}

int storage_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_storage_tables, ARRAY_SIZE(g_storage_tables), argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}
