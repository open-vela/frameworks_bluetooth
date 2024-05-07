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
#ifndef __BT_HCI_H4_H_
#define __BT_HCI_H4_H_

#include <stdint.h>

int bt_sal_hci_transport_init(void);
void bt_sal_hci_transport_recv(void);
int bt_sal_hci_send_packet(uint8_t* buf, uint32_t len);
void bt_sal_hci_transport_cleanup(void);

#endif /* __BT_HCI_H4_H_ */