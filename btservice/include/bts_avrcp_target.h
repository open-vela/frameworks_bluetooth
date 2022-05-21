#ifndef __BTS_AVRCP_TARGET__H__
#define __BTS_AVRCP_TARGET__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>

#include "bts_gap_service.h"
#include "stack_adapter_common.h"

typedef enum{
    PLAY_STATUS_STOPPED = MEDIA_STOPPED,
    PLAY_STATUS_PLAYING,
    PLAY_STATUS_PAUSED,
    PLAY_STATUS_FWD_SEEK,
    PLAY_STATUS_REV_SEEK,
    PLAY_STATUS_ERROR
}play_status_t;

bt_result_code bts_avrcp_target_init(void);
bt_result_code bts_avrcp_notify_play_state_changed(bt_address addr, play_status_t status);
void bts_avrcp_target_cleanup(void);
#endif