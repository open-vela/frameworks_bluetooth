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
#ifndef __BT_ADDR_H__
#define __BT_ADDR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

<<<<<<< HEAD
#define BT_ADDR_LENGTH 6 /*define the address length*/
=======
#define BT_ADDR_LENGTH     6 /*define the address length*/
>>>>>>> bluetooth framework re-implement base
#define BT_ADDR_STR_LENGTH 18

typedef struct bt_addr {
    uint8_t addr[BT_ADDR_LENGTH];
} bt_address_t;

typedef struct bt_le_addr {
    uint8_t addr[BT_ADDR_LENGTH];
    uint8_t addr_type;
} bt_le_address_t;

bool bt_addr_is_empty(bt_address_t *addr);
void bt_addr_set_empty(bt_address_t *addr);
int bt_addr_compare(bt_address_t *a, bt_address_t *b);
int bt_addr_ba2str(bt_address_t *addr, char *str);
int bt_addr_str2ba(const char *str, bt_address_t *addr);
<<<<<<< HEAD
char *bt_addr_str(bt_address_t *addr);
=======
>>>>>>> bluetooth framework re-implement base
void bt_addr_set(bt_address_t *addr, uint8_t *bd);
void bt_addr_swap(bt_address_t *src, bt_address_t *dest);

#ifdef __cplusplus
}
#endif

<<<<<<< HEAD
#endif /* __BT_ADDR_H_ */
=======
#endif /* __BT_ADDR_H_ */
>>>>>>> bluetooth framework re-implement base
