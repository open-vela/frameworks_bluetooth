/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#include <stdlib.h>
#include <string.h>

#include "bt_hid_device.h"
#include "bt_tools.h"

static int register_cmd(void* handle, int argc, char* argv[]);
static int unregister_cmd(void* handle, int argc, char* argv[]);
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int send_report_cmd(void* handle, int argc, char* argv[]);
static int send_keyboard_cmd(void* handle, int argc, char* argv[]);
static int send_mouse_cmd(void* handle, int argc, char* argv[]);
static int send_consumer_cmd(void* handle, int argc, char* argv[]);
static int unplug_cmd(void* handle, int argc, char* argv[]);
static int dump_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_hidd_tables[] = {
    { "register", register_cmd, 0, "\"register HID app: <type>(1:KEYBOARD, 2:MOUSE, 3:KBMS_COMBO) <transport>(0:BLE, 1:BREDR)\"" },
    { "unregister", unregister_cmd, 0, "\"unregister HID app \"" },
    { "connect", connect_cmd, 0, "\"connect HID host param: <address> \"" },
    { "disconnect", disconnect_cmd, 0, "\"disconnect HID host param: <address>\"" },
    { "send_report", send_report_cmd, 0, "\"send report param: <address> <report id> <report data> \"" },
    { "send_keyboard", send_keyboard_cmd, 0, "\"send keyboard report: <address> <modifier key> <normal key>\"" },
    { "send_mouse", send_mouse_cmd, 0, "\"send mouse report: <address> <X axises>(-127~128) <Y axises>(-127~128)\"" },
    { "send_consumer", send_consumer_cmd, 0, "\"send consumer report: <address> <consumer key>\"" },
    { "unplug", unplug_cmd, 0, "\"virtual unplug param: <address> \"" },
    { "dump", dump_cmd, 0, "\"dump HID device current state\"" },
};

static void* hidd_callbacks = NULL;

const static uint8_t s_hid_KB_report_desc[] = {
    0x05, 0x01,
    0x09, 0x06,
    0xA1, 0x01,

    0x05, 0x07,
    0x19, 0xE0,
    0x29, 0xE7,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x08,
    0x75, 0x01,
    0x81, 0x02,

    0x95, 0x01,
    0x75, 0x08,
    0x81, 0x01,

    0x05, 0x08,
    0x19, 0x01,
    0x29, 0x05,
    0x95, 0x05,
    0x75, 0x01,
    0x91, 0x02,
    0x95, 0x01,
    0x75, 0x03,
    0x91, 0x01,

    0x05, 0x07,
    0x19, 0x00,
    0x29, 0x65,
    0x15, 0x00,
    0x25, 0x65,
    0x95, 0x06,
    0x75, 0x08,
    0x81, 0x00,
    0xC0
};

const static uint8_t s_hid_MS_report_desc[] = {
    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x09, 0x01,
    0xA1, 0x00,

    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x05,
    0x81, 0x01,

    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x03,
    0x81, 0x06,
    0xC0,
    0xC0
};

const static uint8_t s_hid_combo_report_desc[] = {
    0x05, 0x0C,
    0x09, 0x01,
    0xA1, 0x01,
    0x85, 0x01,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x01,
    0x75, 0x01,
    0x09, 0xCD,
    0x81, 0x06,
    0x0A, 0x83, 0x01,
    0x81, 0x06,
    0x09, 0xB5,
    0x81, 0x06,
    0x09, 0xB6,
    0x81, 0x06,
    0x09, 0xEA,
    0x81, 0x06,
    0x09, 0xE9,
    0x81, 0x06,
    0x0A, 0x23, 0x02,
    0x81, 0x06,
    0x0A, 0x24, 0x02,
    0x81, 0x06,
    0xC0,

    0x05, 0x01,
    0x09, 0x02,
    0xA1, 0x01,
    0x09, 0x01,
    0xA1, 0x00,
    0x85, 0x02,
    0x05, 0x09,
    0x19, 0x01,
    0x29, 0x03,
    0x15, 0x00,
    0x25, 0x01,
    0x95, 0x03,
    0x75, 0x01,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x05,
    0x81, 0x01,

    0x05, 0x01,
    0x09, 0x30,
    0x09, 0x31,
    0x09, 0x38,
    0x15, 0x81,
    0x25, 0x7F,
    0x75, 0x08,
    0x95, 0x03,
    0x81, 0x06,
    0xC0,
    0xC0
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_hidd_tables); i++) {
        printf("\t%-8s\t%s\n", g_hidd_tables[i].cmd, g_hidd_tables[i].help);
    }
}

static void hidd_app_state_cb(void* cookie, hid_app_state_t state)
{
    PRINT("%s, state: %s", __func__, (state == HID_APP_STATE_REGISTERED) ? "registered" : "not registed");
}

static void hidd_connection_state_cb(void* cookie, bt_address_t* addr, bool le_hid,
    profile_connection_state_t state)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("%s, addr:%s, transport: %s, state:%d", __func__, addr_str, le_hid ? "le" : "br", state);
}

static void hidd_get_report_cb(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint8_t rpt_id, uint16_t buffer_size)
{
    uint8_t rpt_data[] = { 0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 };
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("%s, addr:%s, buffer size: %d", __func__, addr_str, buffer_size);

    if (rpt_id != 0)
        rpt_data[0] = rpt_id;

    bt_hid_device_response_report(cookie, addr, rpt_type, rpt_data, sizeof(rpt_data));
}

static void hidd_set_report_cb(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("%s, addr:%s, report type: %d", __func__, addr_str, rpt_type);
    lib_dumpbuffer("report data:", rpt_data, rpt_size);
    bt_hid_device_report_error(cookie, addr, HID_STATUS_OK);
}

static void hidd_receive_report_cb(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("%s, addr:%s, report type: %d", __func__, addr_str, rpt_type);
    lib_dumpbuffer("report data:", rpt_data, rpt_size);
}

static void hidd_virtual_unplug_cb(void* cookie, bt_address_t* addr)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("%s, addr:%s", __func__, addr_str);
}

enum {
    APP_HID_DEVICE_KEYBOARD = 1,
    APP_HID_DEVICE_MOUSE = 2,
    APP_HID_DEVICE_KBMS_COMBO = 3
};

static int register_cmd(void* handle, int argc, char* argv[])
{
    hid_device_sdp_settings_t hidd_setting;
    const uint8_t* desc_list;
    uint16_t desc_len;
    int app_type;
    int transport;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    app_type = atoi(argv[0]);
    transport = (argc < 2) ? BT_TRANSPORT_BREDR : atoi(argv[1]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    memset(&hidd_setting, 0, sizeof(hid_device_sdp_settings_t));
    hidd_setting.name = "HID_Device_Demo";
    hidd_setting.description = "A demo of HID Device implementation";
    hidd_setting.provider = "Xiaomi Vela";
    hidd_setting.hids_info.attr_mask = HID_ATTR_MASK_VIRTUAL_CABLE | HID_ATTR_MASK_RECONNECT_INITIATE | HID_ATTR_MASK_NORMALLY_CONNECTABLE /* | BTHID_ATTR_MASK_BOOT_DEVICE*/;

    switch (app_type) {
    case APP_HID_DEVICE_KEYBOARD:
        hidd_setting.hids_info.sub_class = (uint8_t)COD_PERIPHERAL_KEYBOARD;
        desc_list = s_hid_KB_report_desc;
        desc_len = sizeof(s_hid_KB_report_desc);
        break;
    case APP_HID_DEVICE_MOUSE:
        hidd_setting.hids_info.sub_class = (uint8_t)COD_PERIPHERAL_POINT;
        desc_list = s_hid_MS_report_desc;
        desc_len = sizeof(s_hid_MS_report_desc);
        break;
    default:
        hidd_setting.hids_info.sub_class = (uint8_t)COD_PERIPHERAL_KEYORPOINT;
        desc_list = s_hid_combo_report_desc;
        desc_len = sizeof(s_hid_combo_report_desc);
        break;
    }

    hidd_setting.hids_info.dsc_list = malloc(desc_len + 3);
    if (!hidd_setting.hids_info.dsc_list) {
        return CMD_ERROR;
    }

    hidd_setting.hids_info.vendor_id = 0x038F;
    hidd_setting.hids_info.product_id = 0x1234;
    hidd_setting.hids_info.version = 0x100;
    hidd_setting.hids_info.dsc_list_length = (uint16_t)(desc_len + 3); /* 3 bytes for Descriptor Type and Length */
    hidd_setting.hids_info.dsc_list[0] = HID_SDP_DESCRIPTOR_REPORT;
    hidd_setting.hids_info.dsc_list[1] = (uint8_t)(desc_len & 0xFF);
    hidd_setting.hids_info.dsc_list[2] = (uint8_t)(desc_len >> 8);
    memcpy(hidd_setting.hids_info.dsc_list + 3, desc_list, desc_len);

    bt_status_t ret = bt_hid_device_register_app(handle, &hidd_setting, transport == BT_TRANSPORT_BLE);
    free(hidd_setting.hids_info.dsc_list);
    if (ret != BT_STATUS_SUCCESS) {
        if (ret == BT_STATUS_NO_RESOURCES) {
            PRINT("HID app has registed, please unregister then try again");
        }
        return CMD_ERROR;
    }

    PRINT("HID device register app, type:%s", argv[0]);

    return CMD_OK;
}

static int unregister_cmd(void* handle, int argc, char* argv[])
{
    bt_status_t ret = bt_hid_device_unregister_app(handle);
    if (ret != BT_STATUS_SUCCESS) {
        if (ret == BT_STATUS_NOT_FOUND) {
            PRINT("HID app isn't registed, please register then try again");
        }
        return CMD_ERROR;
    }

    PRINT("HID device unregister app");

    return CMD_OK;
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_device_connect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("HID device connect host, address:%s", argv[0]);

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_device_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("HID device disconnect host, address:%s", argv[0]);

    return CMD_OK;
}

static void hex2str(char* src_str, uint8_t* dest_buf, uint8_t hex_number)
{
    uint8_t i;
    uint8_t lb, hb;

    for (i = 0; i < hex_number; i++) {
        lb = src_str[(i << 1) + 1];
        hb = src_str[i << 1];
        if (hb >= '0' && hb <= '9') {
            dest_buf[i] = hb - '0';
        } else if (hb >= 'A' && hb < 'G') {
            dest_buf[i] = hb - 'A' + 10;
        } else if (hb >= 'a' && hb < 'g') {
            dest_buf[i] = hb - 'a' + 10;
        } else {
            dest_buf[i] = 0;
        }

        dest_buf[i] <<= 4;
        if (lb >= '0' && lb <= '9') {
            dest_buf[i] += lb - '0';
        } else if (lb >= 'A' && lb < 'G') {
            dest_buf[i] += lb - 'A' + 10;
        } else if (lb >= 'a' && lb < 'g') {
            dest_buf[i] += lb - 'a' + 10;
        }
    }
}

static int send_report_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint8_t report_id;
    uint8_t report_data[8];
    int report_length;
    char* buffer;
    int size;

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    report_id = atoi(argv[1]);
    size = strlen(argv[2]) + 1;
    buffer = (char*)malloc(size);
    if (!buffer)
        return CMD_ERROR;

    memcpy(buffer, argv[2], size);
    buffer[size - 1] = 0;

    report_length = size / 2;
    if (report_length > sizeof(report_data))
        report_length = sizeof(report_data);

    memset(report_data, 0, sizeof(report_data));
    hex2str(buffer, report_data, report_length);

    bt_status_t ret = bt_hid_device_send_report(handle, &addr, report_id, report_data, report_length);
    free(buffer);
    if (ret != BT_STATUS_SUCCESS) {
        return CMD_ERROR;
    }

    return CMD_OK;
}

static int send_keyboard_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint8_t rpt_data[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    rpt_data[0] = strtol(argv[1], NULL, 16);
    rpt_data[2] = strtol(argv[2], NULL, 16);

    PRINT("modifier key: 0x%02X, normal key: 0x%02X", rpt_data[0], rpt_data[2]);
    if (bt_hid_device_send_report(handle, &addr, 0, rpt_data, sizeof(rpt_data)) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    memset(rpt_data, 0, sizeof(rpt_data));
    if (bt_hid_device_send_report(handle, &addr, 0, rpt_data, sizeof(rpt_data)) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int send_mouse_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    int8_t rpt_data[4] = { 0x00, 0x00, 0x00, 0x00 };

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    rpt_data[1] = atoi(argv[1]);
    rpt_data[2] = atoi(argv[2]);

    PRINT("X axises: %d, Y axises: %d", rpt_data[0], rpt_data[2]);
    if (bt_hid_device_send_report(handle, &addr, 0, (uint8_t*)rpt_data, sizeof(rpt_data)) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    memset(rpt_data, 0, sizeof(rpt_data));
    if (bt_hid_device_send_report(handle, &addr, 0, (uint8_t*)rpt_data, sizeof(rpt_data)) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int send_consumer_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint8_t rpt_data[2] = { 0x01, 0x00 };

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    PRINT("%s, address:%s", __func__, argv[0]);
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    rpt_data[1] = strtol(argv[1], NULL, 16);

    PRINT("consumer key: 0x%02X", rpt_data[1]);
    if (bt_hid_device_send_report(handle, &addr, 1, rpt_data, sizeof(rpt_data)) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    rpt_data[1] = 0;
    if (bt_hid_device_send_report(handle, &addr, 1, rpt_data, sizeof(rpt_data)) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int unplug_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hid_device_virtual_unplug(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("HID device virtual unplug success, address:%s", argv[0]);

    return CMD_OK;
}

static int dump_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static const hid_device_callbacks_t hidd_test_cbs = {
    sizeof(hid_device_callbacks_t),
    hidd_app_state_cb,
    hidd_connection_state_cb,
    hidd_get_report_cb,
    hidd_set_report_cb,
    hidd_receive_report_cb,
    hidd_virtual_unplug_cb,
};

int hidd_command_init(void* handle)
{
    hidd_callbacks = bt_hid_device_register_callbacks(handle, &hidd_test_cbs);

    return 0;
}

void hidd_command_uninit(void* handle)
{
    bt_hid_device_unregister_callbacks(handle, hidd_callbacks);
}

int hidd_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_hidd_tables, ARRAY_SIZE(g_hidd_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
