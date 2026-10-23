/* PAN service/SAL type contract (Team 090). Public types from bt_pan.h. */
#ifndef __SAL_PAN_INTERFACE_H
#define __SAL_PAN_INTERFACE_H

#include "bt_pan.h"

/* vtable returned by get_pan_profile_interface(); fields mirror the
 * designated initializers of panInterface in panu_service.c. */
typedef struct {
    size_t size;
    void* (*register_callbacks)(void* remote, const pan_callbacks_t* callbacks);
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bt_status_t (*connect)(bt_address_t* addr, uint8_t dst_role, uint8_t src_role);
    bt_status_t (*disconnect)(bt_address_t* addr);
} pan_interface_t;

bt_status_t bt_sal_pan_init(uint8_t max_conn, pan_role_t local_role);
bt_status_t bt_sal_pan_cleanup(void);
bt_status_t bt_sal_pan_connect(bt_address_t* addr, uint8_t dst_role, uint8_t src_role);
bt_status_t bt_sal_pan_disconnect(bt_address_t* addr);
bt_status_t bt_sal_pan_write(bt_address_t* addr, uint16_t protocol,
    uint8_t* dst_addr, uint8_t* src_addr, uint8_t* data, uint16_t length);
#endif
