/****************************************************************************
 * frameworks/bluetooth/samples/test_gatts.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "bttool_hidd"

#include <debug.h>
#include <nuttx/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bt_tools.h"
#include "btm_hid_device.h"
#include "btm_manager.h"
#include "log.h"

typedef struct {
    struct list_node node;
    bt_address remote_address;
} hidd_device_t;

static btm_interface_t* manager = NULL;
static btm_hid_device_interface_t* hidd_interface = NULL;
static struct list_node hidd_device_list = LIST_INITIAL_VALUE(hidd_device_list);
static void* hidd_handle;
static bool hidd_registered = false;

const static uint8_t s_hidKBReportDesc[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop), */
    0x09, 0x06, /* Usage (Keyboard), */
    0xA1, 0x01, /* Collection (Application), */
    0x05, 0x07, /* Usage Page (Key Codes); */
    0x19, 0xE0, /* Usage Minimum (224), */
    0x29, 0xE7, /* Usage Maximum (231), */
    0x15, 0x00, /* Logical Minimum (0), */
    0x25, 0x01, /* Logical Maximum (1), */
    0x75, 0x01, /* Report Size (1), */
    0x95, 0x08, /* Report Count (8), */
    0x81, 0x02, /* Input (Data, Variable, Absolute), ;Modifier byte */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x08, /* Report Size (8), */
    0x81, 0x01, /* Input (Constant), ;Reserved byte */
    0x95, 0x05, /* Report Count (5), */
    0x75, 0x01, /* Report Size (1), */
    0x05, 0x08, /* Usage Page (Page# for LEDs), */
    0x19, 0x01, /* Usage Minimum (1), */
    0x29, 0x05, /* Usage Maximum (5), */
    0x91, 0x02, /* Output (Data, Variable, Absolute), ;LED report */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x03, /* Report Size (3), */
    0x91, 0x01, /* Output (Constant), ;LED report padding */
    0x95, 0x06, /* Report Count (6), */
    0x75, 0x08, /* Report Size (8), */
    0x15, 0x00, /* Logical Minimum (0), */
    0x25, 0x65, /* Logical Maximum(101), */
    0x05, 0x07, /* Usage Page (Key Codes), */
    0x19, 0x00, /* Usage Minimum (0), */
    0x29, 0x65, /* Usage Maximum (101), */
    0x81, 0x00, /* Input (Data, Array), ;Key arrays (6 bytes) */
    0xC0, /* End Collection */
};

const uint8_t s_hidMouseReportDesc[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop), */
    0x09, 0x02, /* Usage (Mouse), */
    0xA1, 0x01, /* Collection (Application), */
    0x09, 0x01, /* Usage (Pointer), */
    0xA1, 0x00, /* Collection (Physical), */
    0x85, 0x02, /* Report_ID (2), */
    0x05, 0x09, /* Usage Page (Buttons), */
    0x19, 0x01, /* Usage Minimum (01), */
    0x29, 0x03, /* Usage Maximun (03), */
    0x15, 0x00, /* Logical Minimum (0), */
    0x25, 0x01, /* Logical Maximum (1), */
    0x95, 0x03, /* Report Count (3), */
    0x75, 0x01, /* Report Size (1), */
    0x81, 0x02, /* Input (Data, Variable, Absolute),    ;3 button bits */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x05, /* Report Size (5), */
    0x81, 0x01, /* Input (Constant),    ;5 bit padding */

    0x05, 0x01, /* Usage Page (Generic Desktop), */
    0x09, 0x30, /* Usage (X), */
    0x09, 0x31, /* Usage (Y), */
    0x09, 0x38, /* Usage (Wheel), */
    0x15, 0x81, /* Logical Minimum (-127), */
    0x25, 0x7F, /* Logical Maximum (127), */
    0x75, 0x08, /* Report Size (8), */
    0x95, 0x03, /* Report Count (3), */
    0x81, 0x06, /* Input (Data, Variable, Relative),    ;3 position bytes (X & Y & Wheel) */
    0xC0, /* End Collection, */

    0x85, 0x03, /* REPORT ID(3) Function Key */
    0x05, 0x0C, /* Usage Page (Consumer), */
    0x09, 0xE9, /* Usage (Volume Increment) */
    0x09, 0xEA, /* Usage (Volume Decrement) */
    0x0A, 0x24, 0x02, /* Usage (AC Back), */
    0x0A, 0x23, 0x02, /* Usage (AC Home), */
    0x95, 0x04, /* Report Count (4), */
    0x75, 0x01, /* Report Size (1), */
    0x81, 0x02, /* Input (Data, Variable, Absolute), 4 button bits */

    0x95, 0x01, /* REPORT_COUNT (1) */
    0x75, 0x04, /* REPORT_SIZE (4) */
    0x81, 0x01, /* INPUT (Cnst) - Reserved Byte (I)*/
    0xC0 /* End Collection */
};

const static uint8_t s_hidComboReportDesc[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop), */
    0x09, 0x02, /* Usage (Mouse), */
    0xA1, 0x01, /* Collection (Application), */
    0x09, 0x06, /* Usage (Keyboard), */
    0x85, 0x01, /* Report_ID (1), */
    0xA1, 0x00, /* Collection (Physical), */
    0x05, 0x07, /* Usage Page (Key Codes); */
    0x19, 0xE0, /* Usage Minimum (224), */
    0x29, 0xE7, /* Usage Maximum (231), */
    0x15, 0x00, /* Logical Minimum (0), */
    0x25, 0x01, /* Logical Maximum (1), */
    0x75, 0x01, /* Report Size (1), */
    0x95, 0x08, /* Report Count (8), */
    0x81, 0x02, /* Input (Data, Variable, Absolute), ;Modifier byte */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x08, /* Report Size (8), */
    0x81, 0x01, /* Input (Constant), ;Reserved byte */

    0x95, 0x05, /* Report Count (5), */
    0x75, 0x01, /* Report Size (1), */
    0x05, 0x08, /* Usage Page (Page# for LEDs), */
    0x19, 0x01, /* Usage Minimum (1), */
    0x29, 0x05, /* Usage Maximum (5), */
    0x91, 0x02, /* Output (Data, Variable, Absolute), ;LED report */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x03, /* Report Size (3), */
    0x91, 0x01, /* Output (Constant), ;LED report padding */

    0x95, 0x06, /* Report Count (6), */
    0x75, 0x08, /* Report Size (8), */
    0x15, 0x00, /* Logical Minimum (0), */
    0x25, 0x65, /* Logical Maximum(101), */
    0x05, 0x07, /* Usage Page (Key Codes), */
    0x19, 0x00, /* Usage Minimum (0), */
    0x29, 0x65, /* Usage Maximum (101), */
    0x81, 0x00, /* Input (Data, Array), ;Key arrays (6 bytes) */
    0xC0, /* End Collection */

    0x05, 0x01, /* Usage Page (Generic Desktop), */
    0x09, 0x01, /* Usage (Pointer), */
    0x85, 0x02, /* Report_ID (2), */
    0xA1, 0x00, /* Collection (Physical), */
    0x05, 0x09, /* Usage Page (Buttons), */
    0x19, 0x01, /* Usage Minimum (01), */
    0x29, 0x03, /* Usage Maximun (03), */
    0x15, 0x00, /* Logical Minimum (0), */
    0x25, 0x01, /* Logical Maximum (1), */
    0x95, 0x03, /* Report Count (3), */
    0x75, 0x01, /* Report Size (1), */
    0x81, 0x02, /* Input (Data, Variable, Absolute),    ;3 button bits */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x05, /* Report Size (5), */
    0x81, 0x01, /* Input (Constant),    ;5 bit padding */

    0x05, 0x01, /* Usage Page (Generic Desktop), */
    0x09, 0x30, /* Usage (X), */
    0x09, 0x31, /* Usage (Y), */
    0x09, 0x38, /* Usage (Wheel), */
    0x15, 0x81, /* Logical Minimum (-127), */
    0x25, 0x7F, /* Logical Maximum (127), */
    0x75, 0x08, /* Report Size (8), */
    0x95, 0x03, /* Report Count (3), */
    0x81, 0x06, /* Input (Data, Variable, Relative),    ;3 position bytes (X & Y & Wheel) */
    0xC0, /* End Collection, */

    0x05, 0x0C, /* Usage Page (Consumer), */
    0x09, 0x01, /* Usage (Consumer Control), */
    0x85, 0x03, /* Report_ID (3), */
    0xA1, 0x00, /* Collection (Physical), */
    0x0A, 0x23, 0x02, /* Usage (AC Home), */
    0x0A, 0x24, 0x02, /* Usage (AC Back), */
    0x09, 0x40,
    0x09, 0x65, /* Usage (Snapshot) */
    0x95, 0x04, /* Report Count (4), */
    0x75, 0x01, /* Report Size (1), */
    0x81, 0x02, /* Input (Data, Variable, Absolute),    ;3 button bits */

    0x95, 0x01, /* Report Count (1), */
    0x75, 0x04, /* Report Size (4), */
    0x81, 0x01, /* Input (Constant),    ;6 bit padding */
    0xC0, /* End Collection, */

    0xC0 /* End Collection */
};

static hidd_device_t* find_hidd_device(bt_address remote_address)
{
    hidd_device_t* device;
    list_for_every_entry(&hidd_device_list, device, hidd_device_t, node)
    {
        if (!memcmp(device->remote_address, remote_address, sizeof(bt_address))) {
            return device;
        }
    }
    return NULL;
}

static hidd_device_t* add_hidd_device(bt_address remote_address)
{
    hidd_device_t* device = (hidd_device_t*)malloc(sizeof(hidd_device_t));
    if (!device) {
        BT_LOGE("malloc device fail");
        return NULL;
    }

    memset(device, 0, sizeof(hidd_device_t));
    memcpy(device->remote_address, remote_address, sizeof(bt_address));
    list_add_tail(&hidd_device_list, &device->node);
    return device;
}

static bool remove_hidd_device(hidd_device_t* device)
{
    if (!device) {
        return true;
    }
    list_delete(&device->node);
    free(device);
    return true;
}

static void on_hidd_device_state_changed_callback(void* handle, hid_app_state registered)
{
    BT_LOGD("%s registered:%d", __func__, registered);
    hidd_registered = registered;
    if (!registered) {
        BT_LOGD("%s clear hidd device", __func__);
        hidd_device_t* device;
        hidd_device_t* tmp;
        list_for_every_entry_safe(&hidd_device_list, device, tmp, hidd_device_t, node)
        {
            remove_hidd_device(device);
        }
    }
}

static void on_hidd_connection_state_changed_callback(void* handle, bt_address remote_addr, bool le_hid, profile_connection_state state)
{
    BT_LOGD("%s addr:[%s],le_hid:%d,  state:%d", __func__, addr_str(remote_addr), le_hid, state);
    if (state == PROFILE_CONNECTED) {
        hidd_device_t* device = find_hidd_device(remote_addr);
        if (!device) { //Connect from peer
            add_hidd_device(remote_addr);
        }
    } else if (state == PROFILE_DISCONNECTED) {
        hidd_device_t* device = find_hidd_device(remote_addr);
        if (device) {
            remove_hidd_device(device);
        }
    }
}

static bt_hid_device_callbacks hidd_callbacks = {
    .hidd_app_state_changed_cb = on_hidd_device_state_changed_callback,
    .hidd_connection_state_changed_cb = on_hidd_connection_state_changed_callback,
};

typedef enum {
    APP_HID_DEVICE_KEYBOARD = 1,
    APP_HID_DEVICE_MOUSE = 2,
    APP_HID_DEVICE_KBMS_COMBO = 3
} tAPP_HID_DEVICE_TYPE;

static int hidd_register_device(void* handle, int argc, char** argv)
{
    if (!hidd_interface || argc < 1) {
        return -1;
    }

    int dev_type = atoi(argv[0]);
    BT_LOGD("register hid type:%d", dev_type);

    if (hidd_registered) {
        BT_LOGE("hidd has registed, please unregister then try again");
        return 0;
    }
    bt_hidd_sdp_settings_t hids_info;
    const uint8_t* desc_list;
    uint16_t desc_len;
    uint8_t sub_class;
    memset(&hids_info, 0, sizeof(bt_hidd_sdp_settings_t));
    hids_info.name = "BRT_HID_Device_Demo";
    hids_info.description = "A demo of HID Device implementation";
    hids_info.provider = "BARROT Technology Limited";
    hids_info.hids_info.attr_mask = HID_ATTR_MASK_VIRTUAL_CABLE | HID_ATTR_MASK_RECONNECT_INITIATE /* | BTHID_ATTR_MASK_BOOT_DEVICE*/;

    switch (dev_type) {
    case APP_HID_DEVICE_KEYBOARD:
        sub_class = (uint8_t)COD_PERIPHERAL_KEYBOARD;
        desc_list = s_hidKBReportDesc;
        desc_len = sizeof(s_hidKBReportDesc);
        break;
    case APP_HID_DEVICE_MOUSE:
        sub_class = (uint8_t)COD_PERIPHERAL_POINT;
        desc_list = s_hidMouseReportDesc;
        desc_len = sizeof(s_hidMouseReportDesc);
        break;
    default:
        sub_class = (uint8_t)COD_PERIPHERAL_KEYORPOINT;
        desc_list = s_hidComboReportDesc;
        desc_len = sizeof(s_hidComboReportDesc);
        break;
    }
    hids_info.hids_info.sub_class = sub_class;
    hids_info.hids_info.vendor_id = 0x1010; /* Demo only */
    hids_info.hids_info.product_id = 0x0C0D; /* Demo only */
    hids_info.hids_info.version = 0x200; /* Demo only */
    hids_info.hids_info.dsc_list_length = (uint16_t)(desc_len + 3); /* 3 bytes for Descriptor Type and Length */
    hids_info.hids_info.dsc_list = malloc(desc_len + 3);
    hids_info.hids_info.dsc_list[0] = HID_DESC_TYPE_REPORT;
    hids_info.hids_info.dsc_list[1] = (uint8_t)(desc_len & 0xFF);
    hids_info.hids_info.dsc_list[2] = (uint8_t)(desc_len >> 8);
    memcpy(hids_info.hids_info.dsc_list + 3, desc_list, desc_len);
    bt_hidd_qos_settings_t tx_qos, rx_qos;

    bt_result_code ret = hidd_interface->register_device(&hidd_handle, hids_info, tx_qos, rx_qos, &hidd_callbacks);
    free(hids_info.hids_info.dsc_list);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, register_device  ret: %d", ret);
        return 0;
    }
    return 0;
}

static int hidd_unregister_device(void* handle, int argc, char** argv)
{
    bt_result_code ret = hidd_interface->unregister_device(hidd_handle);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, unregister_device  ret: %d", ret);
        return 0;
    }
    return 0;
}

static int hidd_connect(void* handle, int argc, char** argv)
{
    if (!hidd_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("connect remote_addr:[%s]", addr_str(remote_address));
    hidd_device_t* device = find_hidd_device(remote_address);
    if (!device) {
        device = add_hidd_device(remote_address);
    }
    bt_result_code ret = hidd_interface->connect(hidd_handle, device->remote_address);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, connect  ret: %d", ret);
        return 0;
    }
    return 0;
}

static int hidd_disconnect(void* handle, int argc, char** argv)
{
    if (!hidd_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("disconnect remote_addr:[%s]", addr_str(remote_address));
    hidd_device_t* device = find_hidd_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }
    bt_result_code ret = hidd_interface->disconnect(hidd_handle, remote_address);
    if (ret != BT_RESULT_SUCCESS) {
        remove_hidd_device(device);
        BT_LOGD("fail, disconnect  ret: %d", ret);
        return 0;
    }
    return 0;
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

static int hidd_send_report(void* handle, int argc, char** argv)
{
    if (!hidd_interface || argc < 3) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);

    uint8_t report_id = atoi(argv[1]);

    size_t size = strlen(argv[2]) + 1;
    char* buffer = (char*)malloc(size);
    memcpy(buffer, argv[2], size);
    buffer[size] = 0;

    BT_LOGD("report remote_addr:%s, report_id:%d, buffer:%s", addr_str(remote_address), report_id, buffer);
    hidd_device_t* device = find_hidd_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }

    uint8_t buf[8];
    memset(buf, 0, sizeof(buf));
    hex2str(buffer, buf, size / 2);
    bt_result_code ret = hidd_interface->send_report(hidd_handle, remote_address, report_id, buf, size / 2);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, send_report  ret: %d", ret);
        return 0;
    }
    free(buffer);
    return 0;
}

static int hidd_unplug(void* handle, int argc, char** argv)
{
    if (!hidd_interface || argc < 1) {
        return -1;
    }
    bt_address remote_address;
    str2ba(argv[0], remote_address);
    BT_LOGD("unplug remote_addr:[%s]", addr_str(remote_address));
    hidd_device_t* device = find_hidd_device(remote_address);
    if (!device) {
        BT_LOGD("device not found");
        return 0;
    }
    bt_result_code ret = hidd_interface->unplug(hidd_handle, remote_address);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("fail, unplug  ret: %d", ret);
        return 0;
    }
    return 0;
}

static bt_command_t g_hidd_tables[] = {
    { "register", hidd_register_device, "\"hidd register(KEYBOARD = 1,DEVICE_MOUSE = 2,KBMS_COMBO = 3) <type>\"" },
    { "unregister", hidd_unregister_device, "\"hidd unregister\"" },
    { "connect", hidd_connect, "\"hidd connect  :<address>\"" },
    { "disconnect", hidd_disconnect, "\"hidd disconnect :<address>t\"" },
    { "send_report", hidd_send_report, "\"hidd send report test: <address> <report_id> <data>\"" },
    { "unplug", hidd_unplug, "\"hidd unplug  :<address>\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_hidd_tables); i++) {
        printf("\t%-8s\t%s\n", g_hidd_tables[i].cmd, g_hidd_tables[i].help);
    }
}

static struct option gattc_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

int hid_device_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (hidd_interface == NULL) {
        manager = get_bt_manager_interface();
        hidd_interface = get_btm_hid_device_interface(manager);
    }

    while ((opt = getopt_long(argc, argv, "h", gattc_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_hidd_tables); i++) {
            if (strncmp(g_hidd_tables[i].cmd, argv[1], strlen(argv[1])) == 0) {
                if (g_hidd_tables[i].func) {
                    ret = g_hidd_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        usage();
    }

    return 0;
}
