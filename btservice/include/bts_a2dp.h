#pragma once

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_manager.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*a2dp command*/
typedef enum {

    A2DP_COMMAND_CONNECT = 1,
    A2DP_COMMAND_DISCONNECT,
    A2DP_COMMAND_SET_ACTIVE_DEVICE,
    A2DP_COMMAND_IS_PLAYING,
    A2DP_COMMAND_GET_CONNECED_DEVICES,
    A2DP_COMMAND_START_STREAM,
    A2DP_COMMAND_STOP_STREAM,
    A2DP_COMMAND_SET_CONFIG,
    A2DP_COMMAND_SEND_DATA,

    A2DP_COMMAND_MAX_ID,
    A2DP_RESPONSE_CONNECTION_STATE_CHANGED,
    A2DP_RESPONSE_SET_ACTIVE_DEVICE,
    A2DP_RESPONSE_IS_PLAYINg,
    A2DP_RESPONSE_START_STREAM,
    A2DP_RESPONSE_STOP_STRAM,
    A2DP_RESPONSE_SET_CONFIG,
    A2DP_RESPONSE_MAX_ID,

} a2dp_command;