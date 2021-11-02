

#ifndef _SRV_INC_GATT_BT_COMOM_H
#define _SRV_INC_GATT_BT_COMOM_H

#include <stdbool.h>
#include <stdint.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gatt.h"

typedef SERVICE_SCAN_PARAMS_S scan_settings_t;
typedef SERVICE_BLE_SCAN_FILTER_S scan_filter_t;
typedef SERVICE_SCAN_RESULT_DATA_S scan_result_t;

typedef BD_ADDR bd_addr_t;
typedef BT_UUID_T bt_uuid_t;

typedef SERVICE_PROFILE_CONNECTION_STATE bt_state_t;

typedef SERVICE_SCAN_ADV_PARAMS_S advertise_param_t;

typedef SERVICE_GATT_ELEMENT_S gatt_element_t;
typedef SERVICE_GATT_RESPONSE_S gatt_response_t;

typedef SERVICE_GATT_STATUS gatt_service_status_t;

typedef SERVICE_BLE_PHY_TYPE ble_phy_type_t;

typedef SERVICE_BT_STACK_STATE stack_state_t;

typedef GATT_SERVER_CALLBACKS_S stack_gatt_server_callbacks;
#endif