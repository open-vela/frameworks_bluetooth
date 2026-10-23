/* pan_engine.h */
#ifndef PAN_ENGINE_H
#define PAN_ENGINE_H

#include "bt_addr.h"
#include "bt_pan.h"

void pan_engine_start(void);
void pan_engine_on_bond_state(bt_address_t *addr, int bonded);
void pan_engine_on_pan_state(profile_connection_state_t st);

#endif
