
#pragma once

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_manager.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*avrcp command*/
typedef enum {

    AVRCP_COMMAND_NOTIFY_PLAY_STATUS_STATE_CHANGED = 1,
    AVRCP_COMMAND_NOTIFY_TRACK_CHANGED,
    AVRCP_COMMAND_SEND_DATA,
    AVRCP_COMMAND_MAX_ID,

    AVRCP_RESPONSE_CONNECTION_STATE_CHANGED,
    AVRCP_RESPONSE_REGISTER_NOTIFICATION_REQUEST,
    AVRCP_RESPONSE_REQUEST_PLAY_STATUS,
    AVRCP_RESPONSE_MAX_ID,

} avrcp_command;