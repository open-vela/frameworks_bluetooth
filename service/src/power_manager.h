/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __BT_POWER_MANAGER_H__
#define __BT_POWER_MANAGER_H__

#include <stdint.h>

typedef struct {
    uint16_t max;
    uint16_t min;
    uint16_t attempt;
    uint16_t timeout;
    uint8_t mode;
} bt_pm_mode_t;

#endif /* __BT_POWER_MANAGER_H__ */
