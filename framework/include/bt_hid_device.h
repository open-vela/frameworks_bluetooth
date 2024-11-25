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

#ifndef __BT_HID_DEVICE_H__
#define __BT_HID_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "bt_addr.h"
#include "bt_device.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/* * Descriptor types in the SDP record */
#define HID_SDP_DESCRIPTOR_REPORT (0x22)
#define HID_SDP_DESCRIPTOR_PHYSICAL (0x23)

/* * HID supported features - bit mask */
#define HID_ATTR_MASK_VIRTUAL_CABLE 0x0001
#define HID_ATTR_MASK_RECONNECT_INITIATE 0x0002
#define HID_ATTR_MASK_BOOT_DEVICE 0x0004
#define HID_ATTR_MASK_BATTERY_POWER 0x0010
#define HID_ATTR_MASK_REMOTE_WAKE 0x0020
#define HID_ATTR_MASK_SUPERVISION_TIMEOUT 0x0080
#define HID_ATTR_MASK_NORMALLY_CONNECTABLE 0x0100
#define HID_ATTR_MASK_SSR_MAX_LATENCY 0x0200
#define HID_ATTR_MASK_SSR_MIN_TIMEOUT 0x0400
#define HID_ATTR_MASK_BREDR 0x8000

/**
 * @brief hid descriptor information
 *
 */
typedef struct {
    uint32_t attr_mask; /* BTHID_ATTR_MASK_VIRTUAL_CABLE etc. */
    uint8_t sub_class;
    uint8_t country_code;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t version;
    uint16_t supervision_timeout;
    uint16_t ssr_max_latency;
    uint16_t ssr_min_timeout;
    uint16_t dsc_list_length; /* Length of desc_list */
    uint8_t pad[4];
    uint8_t* dsc_list; /* List of descriptors. Each descriptor is constructed as: Type(1 Byte), Length(2 Bytes, Little Endian), Values(Length Bytes) */
} hid_info_t;

/**
 * @brief hid device settings for SDP server
 *
 */
typedef struct {
    const char* name;
    const char* description;
    const char* provider;
    hid_info_t hids_info;
} hid_device_sdp_settings_t;

/**
 * @brief hid report type
 *
 */
typedef enum {
    HID_REPORT_RESRV, /* reserved */
    HID_REPORT_INPUT, /* input report */
    HID_REPORT_OUTPUT, /* output report */
    HID_REPORT_FEATURE, /* feature report */
} hid_report_type_t;

/**
 * @brief hid status code
 *
 */
typedef enum {
    HID_STATUS_OK = 0,
    HID_STATUS_HANDSHAKE_NOT_READY,
    HID_STATUS_HANDSHAKE_INVALID_REPORT_ID,
    HID_STATUS_HANDSHAKE_UNSUPPORTED_REQ,
    HID_STATUS_HANDSHAKE_INVALID_PARAM,
    HID_STATUS_HANDSHAKE_UNSPECIFIED_ERROR,
    HID_STATUS_UNSPECIFIED_ERROR,
    HID_ERROR_SDP,
    HID_ERROR_SET_PROTOCOL,
    HID_ERROR_DATABASE_FULL,
    HID_ERROR_DEVICE_TYPE_UNSUPPORTED,
    HID_ERROR_NO_RESOURCES,
    HID_ERROR_AUTHENTICATION_FAILED,
    HID_ERROR_OPERATION_NOT_ALLOWED,
} hid_status_error_t;

/**
 * @brief hid app state
 *
 */
typedef enum {
    HID_APP_STATE_NOT_REGISTERED,
    HID_APP_STATE_REGISTERED,
} hid_app_state_t;

/**
 * @brief hid device app state callback
 *
 * @param cookie - callback cookie.
 * @param state - hid app state
 */
typedef void (*hidd_app_state_callback)(void* cookie, hid_app_state_t state);

/**
 * @brief hid device connection state callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer device.
 * @param le_hid - TRUE is le link. FALSE is BREDR link
 * @param state - hid connection state
 */
typedef void (*hidd_connection_state_callback)(void* cookie, bt_address_t* addr, bool le_hid,
    profile_connection_state_t state);

/**
 * @brief callback for get the specified report from app
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer device.
 * @param rpt_type - report type
 * @param rpt_id - report id
 * @param buffer_size - max size to return
 */
typedef void (*hidd_get_report_callback)(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint8_t rpt_id, uint16_t buffer_size);

/**
 * @brief callback for set the specified report from app
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer device.
 * @param rpt_type - report type
 * @param rpt_size - size of the report data
 * @param rpt_data - report data
 */
typedef void (*hidd_set_report_callback)(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data);

/**
 * @brief callback for receiving reports from host
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer device.
 * @param rpt_type - report type
 * @param rpt_size - size of the report data
 * @param rpt_data - report data
 */
typedef void (*hidd_receive_report_callback)(void* cookie, bt_address_t* addr, uint8_t rpt_type,
    uint16_t rpt_size, uint8_t* rpt_data);

/**
 * @brief hid device virtual cable unplug callback
 *
 * @param cookie - callback cookie.
 * @param addr - address of peer device.
 */
typedef void (*hidd_virtual_unplug_callback)(void* cookie, bt_address_t* addr);

/**
 * @brief hid device event callbacks structure
 *
 */
typedef struct {
    size_t size;
    hidd_app_state_callback app_state_cb;
    hidd_connection_state_callback connection_state_cb;
    hidd_get_report_callback get_report_cb;
    hidd_set_report_callback set_report_cb;
    hidd_receive_report_callback receive_report_cb;
    hidd_virtual_unplug_callback virtual_unplug_cb;
} hid_device_callbacks_t;

/**
 * @brief Register callback functions to hid device service
 *
 * @param ins - bluetooth client instance.
 * @param callbacks - hid device callback functions.
 * @return void* - callback cookie, NULL on failure.
 */
void* BTSYMBOLS(bt_hid_device_register_callbacks)(bt_instance_t* ins, const hid_device_callbacks_t* callbacks);

/**
 * @brief Unregister hid device callback function
 *
 * @param ins - bluetooth client instance.
 * @param cookie - callbacks cookie.
 * @return true - on callback unregister success
 * @return false - on callback cookie not found
 */
bool BTSYMBOLS(bt_hid_device_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Register hid app
 *
 * @param ins - bluetooth client instance.
 * @param sdp_setting - hid device sdp setting.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_register_app)(bt_instance_t* ins, hid_device_sdp_settings_t* sdp_setting, bool le_hid);

/**
 * @brief Unregister hid app
 *
 * @param ins - bluetooth client instance.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_unregister_app)(bt_instance_t* ins);

/**
 * @brief Connect to hid host
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect to hid host
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Send report to hid host
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param rpt_id - report id.
 * @param rpt_data - report data.
 * @param rpt_size - size of the report data.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_send_report)(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_id, uint8_t* rpt_data, int rpt_size);

/**
 * @brief Response report to the Host using GET_REPORT command
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param rpt_type - report type.
 * @param rpt_data - report data.
 * @param rpt_size - size of the report data.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_response_report)(bt_instance_t* ins, bt_address_t* addr, uint8_t rpt_type, uint8_t* rpt_data, int rpt_size);

/**
 * @brief Notifies status to the Host using SET_REPORT command
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @param error - error code.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_report_error)(bt_instance_t* ins, bt_address_t* addr, hid_status_error_t error);

/**
 * @brief Virtual unplug the current hid host
 *
 * @param ins - bluetooth client instance.
 * @param addr - address of peer device.
 * @return bt_status_t - BT_STATUS_SUCCESS on success, a negated errno value on failure.
 */
bt_status_t BTSYMBOLS(bt_hid_device_virtual_unplug)(bt_instance_t* ins, bt_address_t* addr);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HID_DEVICE_H__ */
