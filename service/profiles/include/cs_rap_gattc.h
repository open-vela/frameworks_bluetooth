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
#ifndef __CS_RAP_GATTC_H__
#define __CS_RAP_GATTC_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_addr.h"
#include "bt_cs.h"
#include "bt_gatt_defs.h"
#include "bt_uuid.h"

/**
 * @brief RAS Control Point Op Codes
 */
#define RAS_CP_OP_GET_RANGING_DATA              0x00
#define RAS_CP_OP_ACK_RANGING_DATA              0x01
#define RAS_CP_OP_RETRIEVE_LOST_DATA_SEG        0x02
#define RAS_CP_OP_ABORT_OPERATION               0x03
#define RAS_CP_OP_SET_FILTER                    0x04

/**
 * @brief RAS Control Point Response Op Codes
 */
#define RAS_CP_RSP_COMPLETE_RANGING_DATA        0x00
#define RAS_CP_RSP_COMPLETE_LOST_DATA_SEG       0x01
#define RAS_CP_RSP_RESPONSE_CODE                0x02

/**
 * @brief RAS Control Point Response Code values
 */
#define RAS_RSP_SUCCESS                         0x01
#define RAS_RSP_OP_NOT_SUPPORTED                0x02
#define RAS_RSP_INVALID_OPERAND                 0x03
#define RAS_RSP_ABORT_UNSUCCESSFUL              0x04
#define RAS_RSP_PROCEDURE_NOT_COMPLETED         0x05
#define RAS_RSP_SERVER_BUSY                     0x06
#define RAS_RSP_NO_RECORDS_FOUND                0x07

/**
 * @brief RAP GATTC Characteristic Handles
 */
typedef struct {
    uint16_t features_handle;
    uint16_t real_time_data_handle;
    uint16_t on_demand_data_handle;
    uint16_t control_point_handle;
    uint16_t data_ready_handle;
    uint16_t data_overwritten_handle;
    uint16_t ranging_counter_handle;
} cs_rap_gattc_handles_t;

/**
 * @brief RAP GATTC Callbacks
 */
typedef struct {
    void (*on_connected)(bt_address_t* addr, uint16_t conn_id);
    void (*on_disconnected)(bt_address_t* addr, uint16_t conn_id);
    void (*on_discover_complete)(bt_address_t* addr, bool success, cs_rap_gattc_handles_t* handles);
    void (*on_features_read)(bt_address_t* addr, uint32_t features);
    void (*on_data_ready)(bt_address_t* addr, uint16_t ranging_counter);
    void (*on_ranging_data)(bt_address_t* addr, uint8_t* data, uint16_t len);
    void (*on_control_point_rsp)(bt_address_t* addr, uint8_t opcode, uint8_t* params, uint16_t len);
    void (*on_data_overwritten)(bt_address_t* addr, uint16_t ranging_counter);
    void (*on_write_complete)(bt_address_t* addr, uint16_t handle, gatt_status_t status);
} cs_rap_gattc_callbacks_t;

/**
 * @brief Initialize RAP GATTC module
 *
 * @param callbacks Callback functions
 * @return 0 on success, negative error code on failure
 */
int cs_rap_gattc_init(const cs_rap_gattc_callbacks_t* callbacks);

/**
 * @brief Deinitialize RAP GATTC module
 */
void cs_rap_gattc_deinit(void);

/**
 * @brief Connect to remote RAS server
 *
 * @param addr Remote device address
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_connect(bt_address_t* addr);

/**
 * @brief Disconnect from remote RAS server
 *
 * @param addr Remote device address
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_disconnect(bt_address_t* addr);

/**
 * @brief Discover RAS service and characteristics
 *
 * @param addr Remote device address
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_discover(bt_address_t* addr);

/**
 * @brief Read RAS Features characteristic
 *
 * @param addr Remote device address
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_read_features(bt_address_t* addr);

/**
 * @brief Enable real-time ranging data notifications/indications
 *
 * @param addr   Remote device address
 * @param enable true to enable, false to disable
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_enable_real_time_data(bt_address_t* addr, bool enable);

/**
 * @brief Enable on-demand ranging data notifications/indications
 *
 * @param addr   Remote device address
 * @param enable true to enable, false to disable
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_enable_on_demand_data(bt_address_t* addr, bool enable);

/**
 * @brief Enable data ready notifications/indications
 *
 * @param addr   Remote device address
 * @param enable true to enable, false to disable
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_enable_data_ready(bt_address_t* addr, bool enable);

/**
 * @brief Enable control point indications
 *
 * @param addr   Remote device address
 * @param enable true to enable, false to disable
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_enable_control_point(bt_address_t* addr, bool enable);

/**
 * @brief Write Get Ranging Data command to control point
 *
 * @param addr            Remote device address
 * @param ranging_counter Ranging counter to retrieve
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_get_ranging_data(bt_address_t* addr, uint16_t ranging_counter);

/**
 * @brief Write ACK Ranging Data command to control point
 *
 * @param addr            Remote device address
 * @param ranging_counter Ranging counter to acknowledge
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_ack_ranging_data(bt_address_t* addr, uint16_t ranging_counter);

/**
 * @brief Write Retrieve Lost Data Segments command to control point
 *
 * @param addr            Remote device address
 * @param ranging_counter Ranging counter
 * @param first_seg_idx   First segment index
 * @param last_seg_idx    Last segment index
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_retrieve_lost_segments(bt_address_t* addr, uint16_t ranging_counter,
    uint8_t first_seg_idx, uint8_t last_seg_idx);

/**
 * @brief Write Abort Operation command to control point
 *
 * @param addr Remote device address
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_abort_operation(bt_address_t* addr);

/**
 * @brief Write Set Filter command to control point
 *
 * @param addr   Remote device address
 * @param config Filter configuration
 * @return bt_status_t
 */
bt_status_t cs_rap_gattc_set_filter(bt_address_t* addr, const cs_filter_config_t* config);

#endif /* __CS_RAP_GATTC_H__ */
