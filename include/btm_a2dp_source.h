#pragma once

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_manager.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
typedef enum {
    BTA2dpPlaying,
    BTA2dpPaused,
    BTA2dpStop,
}bt_a2dp_play_state;

typedef struct {
    uint16_t data_length;
    uint8_t data[0];
} bt_a2dp_packet_t;

/* Stream config */
typedef struct {
    uint32_t sample_rate;
    uint8_t codec;
    uint8_t channel;
    uint8_t bitWidth;
    uint8_t reserved;
} bt_a2dp_stream_config_t;

typedef struct 
{
  /* data */
  bt_result_code (*connection_state_change) (bt_address address, bt_connection_state state);
  bt_result_code (*play_state_change) (bt_address address, bt_a2dp_play_state state);
}bt_a2dp_source_callbacks_t;

/** BT AV A2DP Source interface.
 */
typedef struct {
  /** set to sizeof(btav_source_interface_t) */
  size_t size;
  /**
   * Register the BtAv callbacks.
   */
  bt_result_code (*init)(
      void *handle, bt_a2dp_source_callbacks_t* callbacks, bt_a2dp_stream_config_t stream_config);

  bt_result_code (*deinit)(void *handle);

  /** connect to remote device */
  bt_result_code (*connect)(void *handle, const bt_address  address);

  /** dis-connect from remote device */
  bt_result_code (*disconnect)(void *handle, const bt_address address);

  /** sets the connected device as active */
  bt_result_code (*set_active_device)(void *handle, const bt_address address);

  bt_result_code (*get_connected_device)(void *handle, const bt_address address);

  /** start stream */
  bt_result_code (*start_stream)(void *handle, const bt_address bd_addr);

  /** start stream */
  bt_result_code (*stop_ststream)(void *handle, const bt_address bd_addr);

  /** start stream */
  bt_result_code (*is_playing)(void *handle, const bt_address bd_addr);

  /** configure the codecs settings preferences */
  bt_result_code (*config_codec)(
      void *handle, const bt_address address, bt_a2dp_stream_config_t streamConfig);

  bt_result_code (*send_data)(void *handle, const bt_address bd_addr, bt_a2dp_packet_t data);

} bt_a2dp_source_interface_t;