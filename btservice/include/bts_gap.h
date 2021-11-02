#pragma once

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include "btm_gap.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef enum {

    SET_ADDRESS = 0,
    GET_ADDRESS,
    SET_LOCAL_IO_CAPABILITY,
    GET_LOCAL_IO_CAPABILITY,
    SET_LOCAL_NAME,
    GET_LOCAL_NAME,
    SET_LOCAL_DEVICE_CLASS,
    GET_LOCAL_DEVICE_CLASS,
    GET_REMOTE_NAME,
    GET_CONNECTED_STATE,
    CONNECT_ALL,
    DISCONNECT_ALL,
    GET_BOND_STATE,
    CREATE_BOND_STATE,
    CANCEL_BOND,
    REMOVE_BOND,
    START_DISCONVERY,
    SET_SCAN_MODE,
    GAP_COMMAND_MAX_ID,

    SET_ADDRESS_RESPONSE,
    GET_ADDRESS_RESPONSE,
    SET_LOCAL_IO_CAPABILITY_RESPONSE,
    GET_LOCAL_IO_CAPABILITY_RESPONSE,
    SET_LOCAL_NAME_RESPONSE,
    GET_LOCAL_NAME_RESPONSE,
    SET_LOCAL_DEVICE_CLASS_RESPONSE,
    GET_LOCAL_DEVICE_CLASS_RESPONSE,
    REMOTE_NAME_CHANGED,
    GET_CONNECTED_STATE_RESPONSE,
    CONNECT_ALL_RESPONSE,
    DISCONNECT_ALL_RESPONSE,
    GET_BOND_STATE_RESPONSE,
    BOND_STATE_CHANGED,
    START_DISCONVERY_RESPONSE,
    SCAN_MODE_CHANGED,
    GAP_RESPONSE_MAX_ID,
} gap_command;



void bts_common_register_callback(bts_gap_callback_t* cb);
void bts_common_init(void);
bt_result_code gap_init(bts_gap_callback_t* cb);
bt_result_code gap_enable(void);
bt_result_code gap_disable(bool normal_disable);
stack_state_t gap_get_stack_state(void);


