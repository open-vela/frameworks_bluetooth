/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#ifndef __CS_SERVICE_H__
#define __CS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_cs.h"

/* cs interface structure */
typedef struct {
    size_t size;

    /**
     * @brief Register the cs event callback
     * @param[in] callbacks  cs event callback function.
     */
    void* (*register_callbacks)(void* remote, const cs_callbacks_t* callbacks);

    /**
     * @brief Unregister the cs event callback
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bt_status_t (*start_distance_measurement)( bt_distance_measurement_params_t* params);
    
    bt_status_t (*stop_distance_measurement)(bt_address_t* addr, int method, bool timeout);

} cs_interface_t;

/*
 * register profile to service manager
 */
void register_cs_service(void);

#endif /* __CS_SERVICE_H__ */
