/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#ifndef __BT_HID_HOST_H__
#define __BT_HID_HOST_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_status.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif /* BIT */

/* Operation mode (bitwise, can be combined) */
#define BT_HID_HOST_MODE_DEFAULT 0x00 /* GATT only */
#define BT_HID_HOST_MODE_SCI BIT(0) /* GATT + SCI */
#define BT_HID_HOST_MODE_ISO BIT(1) /* GATT + ISO */

/* Performance level */
#define BT_HID_HOST_LEVEL_LOW 0x00 /* Power saving */
#define BT_HID_HOST_LEVEL_MEDIUM 0x01 /* Balanced */
#define BT_HID_HOST_LEVEL_HIGH 0x02 /* Performance */
#define BT_HID_HOST_LEVEL_AUTO 0x03 /* Adaptive/coexistence */

typedef struct {
    size_t size;
    void (*connection_state_cb)(void* cookie, bt_address_t* addr,
        bt_transport_t transport, profile_connection_state_t state);
    void (*report_map_cb)(void* cookie, bt_address_t* addr,
        uint8_t service_index, const uint8_t* data, uint16_t len);
    void (*input_report_cb)(void* cookie, bt_address_t* addr,
        uint8_t service_index, uint8_t report_id,
        const uint8_t* data, uint16_t len);
    void (*get_report_cb)(void* cookie, bt_address_t* addr,
        uint8_t report_id, uint8_t report_type,
        const uint8_t* data, uint16_t len);
    void (*pnp_id_cb)(void* cookie, bt_address_t* addr,
        uint8_t vid_src, uint16_t vid, uint16_t pid, uint16_t version);
    void (*battery_level_cb)(void* cookie, bt_address_t* addr,
        uint8_t bat_index, uint8_t level);
    void (*mode_changed_cb)(void* cookie, bt_address_t* addr,
        uint8_t mode, int status);
} hid_host_callbacks_t;

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

/**
 * @brief Register HID host callbacks.
 * @param ins    - Bluetooth instance.
 * @param callbacks - Callback structure.
 * @return Cookie for unregister, or NULL on failure.
 */
void* BTSYMBOLS(bt_hid_host_register_callbacks)(bt_instance_t* ins, const hid_host_callbacks_t* callbacks);

/**
 * @brief Unregister HID host callbacks.
 * @param ins    - Bluetooth instance.
 * @param cookie - Cookie returned by register.
 * @return true on success, false on failure.
 */
bool BTSYMBOLS(bt_hid_host_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief Connect to a remote HID device.
 * @param ins       - Bluetooth instance.
 * @param addr      - Remote device address.
 * @param transport - BT_TRANSPORT_BLE (HOGP). BT_TRANSPORT_BREDR not yet supported.
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_connect)(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport);

/**
 * @brief Disconnect from a remote HID device.
 * @param ins  - Bluetooth instance.
 * @param addr - Remote device address.
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Get a report from the remote HID device.
 * @param ins         - Bluetooth instance.
 * @param addr        - Remote device address.
 * @param report_id   - Report ID.
 * @param report_type - Report type (input/output/feature).
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_get_report)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type);

/**
 * @brief Set a report on the remote HID device.
 * @param ins         - Bluetooth instance.
 * @param addr        - Remote device address.
 * @param report_id   - Report ID.
 * @param report_type - Report type (input/output/feature).
 * @param data        - Report data.
 * @param len         - Data length.
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_set_report)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t report_id, uint8_t report_type,
    const uint8_t* data, uint16_t len);

/**
 * @brief Set protocol mode on the remote HID device.
 * @param ins           - Bluetooth instance.
 * @param addr          - Remote device address.
 * @param protocol_mode - Protocol mode (report/boot).
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_set_protocol)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t protocol_mode);

/**
 * @brief Send suspend to the remote HID device.
 * @param ins  - Bluetooth instance.
 * @param addr - Remote device address.
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_suspend)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Send exit-suspend to the remote HID device.
 * @param ins  - Bluetooth instance.
 * @param addr - Remote device address.
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_exit_suspend)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set operation mode and performance level.
 * @param ins   - Bluetooth instance.
 * @param addr  - Remote device address.
 * @param mode  - Operation mode (BT_HID_HOST_MODE_*), bitwise combinable.
 * @param level - Performance level (BT_HID_HOST_LEVEL_*).
 * @return BT_STATUS_SUCCESS on request accepted.
 */
bt_status_t BTSYMBOLS(bt_hid_host_set_mode)(bt_instance_t* ins, bt_address_t* addr,
    uint8_t mode, uint8_t level);

#ifdef __cplusplus
}
#endif

#endif /* __BT_HID_HOST_H__ */
