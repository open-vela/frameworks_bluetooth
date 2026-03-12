/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#ifndef __Z_API_EXT_ADV_H__
#define __Z_API_EXT_ADV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "z_api.h"

/* Forward declarations */
struct bt_le_adv_param;
struct bt_le_ext_adv_cb;
struct bt_le_ext_adv;
struct bt_le_ext_adv_start_param;
struct bt_data;

/* Extended Advertising APIs */
int z_api(bt_le_ext_adv_create)(const struct bt_le_adv_param *param,
                                const struct bt_le_ext_adv_cb *cb,
                                struct bt_le_ext_adv **out_adv);
int z_api(bt_le_ext_adv_delete)(struct bt_le_ext_adv *adv);
int z_api(bt_le_ext_adv_start)(struct bt_le_ext_adv *adv,
                               const struct bt_le_ext_adv_start_param *param);
int z_api(bt_le_ext_adv_stop)(struct bt_le_ext_adv *adv);
int z_api(bt_le_ext_adv_update_param)(struct bt_le_ext_adv *adv,
                                      const struct bt_le_adv_param *param);
int z_api(bt_le_ext_adv_set_data)(struct bt_le_ext_adv *adv,
                                  const struct bt_data *ad, size_t ad_len,
                                  const struct bt_data *sd, size_t sd_len);
int z_api(bt_le_adv_set_enable_ext)(struct bt_le_ext_adv *adv, bool enable,
                                    const struct bt_le_ext_adv_start_param *param);

#ifdef __cplusplus
}
#endif

#endif /* __Z_API_EXT_ADV_H__ */
