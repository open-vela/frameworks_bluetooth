/* SPDX-License-Identifier: Apache-2.0 */
#ifndef __SERVICE_TRACE_H
#define __SERVICE_TRACE_H
#include <stdint.h>
struct service_trace_record { uint32_t seq; uint16_t stage; int16_t status; int32_t state; uint32_t event; uintptr_t thread_id; uint64_t at_us; };
void service_trace_reset(void);
void service_trace_note(uint16_t stage, int status, int state, uint32_t event);
uint32_t service_trace_copy(struct service_trace_record *out, uint32_t cap, uint32_t *dropped);
#endif
