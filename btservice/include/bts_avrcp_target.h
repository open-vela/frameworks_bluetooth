#ifndef __BTS_AVRCP_TARGET__H__
#define __BTS_AVRCP_TARGET__H__
/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>

#include "btm_avrcp.h"
#include "bts_gap_service.h"

bt_result_code bts_avrcp_target_init(void);
bt_result_code bts_avrcp_get_play_status_response(bt_address addr, play_status_t status, uint32_t song_len, uint32_t song_pos);
bt_result_code bts_avrcp_notify_play_state_changed(bt_address addr, play_status_t status);
bt_result_code bts_avrcp_notify_volume_changed(bt_address addr, uint8_t volume);
void bts_avrcp_set_callbacks(avrcp_tg_callbacks_t *cbs);
void bts_avrcp_target_cleanup(void);
bt_result_code avrcp_target_service_start(void);
void avrcp_target_service_stop(void);
const avrcp_tg_interface_t *get_avrcp_tg_service_interface(void);
#endif