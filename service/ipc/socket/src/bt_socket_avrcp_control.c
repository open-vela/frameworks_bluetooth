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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "bt_internal.h"

#include "avrcp_control_service.h"
#include "bluetooth.h"
#include "bt_avrcp_control.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (ins->avrcp_control_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, profile_connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.avrcp_control_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.avrcp_control_cb._on_connection_state_changed.state = state;
    bt_socket_server_send(ins, &packet, BT_AVRCP_CONTROL_ON_CONNECTION_STATE_CHANGED);
}

static void on_get_element_attribute_cb(void* cookie, bt_address_t* addr, uint8_t attrs_count, avrcp_element_attr_val_t* attrs)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.addr, addr, sizeof(bt_address_t));
    packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.attrs_count = attrs_count;

    for (int i = 0; i < attrs_count; i++) {
        packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.types[i] = attrs[i].attr_id;
        packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.chr_sets[i] = attrs[i].chr_set;
        switch (attrs[i].attr_id) {
        case AVRCP_ATTR_TITLE:
            if (attrs[i].text == NULL) {
                BT_LOGD("%s, title is null", __func__);
                packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.title[0] = '\0';
                break;
            }

            BT_LOGD("%s, title:%s", __func__, (char*)attrs[i].text);
            strlcpy((char*)packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.title, (char*)attrs[i].text, MIN(strlen((char*)attrs[i].text) + 1, AVRCP_ATTR_MAX_TIELE_LEN));
            break;
        case AVRCP_ATTR_ARTIST_NAME:
            if (attrs[i].text == NULL) {
                BT_LOGD("%s, artist is null", __func__);
                packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.artist[0] = '\0';
                break;
            }

            BT_LOGD("%s, artist:%s", __func__, (char*)attrs[i].text);
            strlcpy((char*)packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.artist, (char*)attrs[i].text, MIN(strlen((char*)attrs[i].text) + 1, AVRCP_ATTR_MAX_ARTIST_LEN));
            break;
        case AVRCP_ATTR_ALBUM_NAME:
            packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.album[0] = '\0';
            break;
        case AVRCP_ATTR_TRACK_NUMBER:
            packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.track_number[0] = '\0';
            break;
        case AVRCP_ATTR_TOTAL_NUMBER_OF_TRACKS:
            packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.total_track_number[0] = '\0';
            break;
        case AVRCP_ATTR_GENRE:
            packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.gener[0] = '\0';
            break;
        case AVRCP_ATTR_PLAYING_TIME_MS:
            packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.playing_time_ms[0] = '\0';
            break;
        case AVRCP_ATTR_COVER_ART_HANDLE:
            packet.avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.cover_art_handle[0] = '\0';
            break;
        default:
            break;
        }
    }

    bt_socket_server_send(ins, &packet, BT_AVRCP_CONTROL_ON_GET_ELEMENT_ATTRIBUTES_REQUEST);
}

const static avrcp_control_callbacks_t g_avrcp_control_cbs = {
    .size = sizeof(avrcp_control_callbacks_t),
    .connection_state_cb = on_connection_state_changed_cb,
    .get_element_attribute_cb = on_get_element_attribute_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_avrcp_control_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    avrcp_control_interface_t* profile;

    switch (packet->code) {
    case BT_AVRCP_CONTROL_REGISTER_CALLBACKS:
        if (ins->avrcp_control_cookie == NULL) {
            profile = (avrcp_control_interface_t*)service_manager_get_profile(PROFILE_AVRCP_CT);
            if (profile) {
                ins->avrcp_control_cookie = profile->register_callbacks(ins, &g_avrcp_control_cbs);
                if (ins->avrcp_control_cookie) {
                    packet->avrcp_control_r.status = BT_STATUS_SUCCESS;
                } else {
                    packet->avrcp_control_r.status = BT_STATUS_NO_RESOURCES;
                }
            } else {
                packet->avrcp_control_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            }
        } else {
            packet->avrcp_control_r.status = BT_STATUS_BUSY;
        }
        break;
    case BT_AVRCP_CONTROL_UNREGISTER_CALLBACKS:
        if (ins->avrcp_control_cookie) {
            profile = (avrcp_control_interface_t*)service_manager_get_profile(PROFILE_AVRCP_CT);
            if (profile)
                profile->unregister_callbacks((void**)&ins, ins->avrcp_control_cookie);
            ins->avrcp_control_cookie = NULL;
            packet->avrcp_control_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->avrcp_control_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case BT_AVRCP_CONTROL_GET_ELEMENT_ATTRIBUTES:
        packet->avrcp_control_r.status = BTSYMBOLS(bt_avrcp_control_get_element_attributes)(ins,
            &packet->avrcp_control_pl._bt_avrcp_control_get_element_attribute.addr);
        break;
    default:
        break;
    }
}

#endif

int bt_socket_client_avrcp_control_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_AVRCP_CONTROL_ON_CONNECTION_STATE_CHANGED:
        CALLBACK_FOREACH(CBLIST, avrcp_control_callbacks_t,
            connection_state_cb,
            &packet->avrcp_control_cb._on_connection_state_changed.addr,
            packet->avrcp_control_cb._on_connection_state_changed.state);
        break;
    case BT_AVRCP_CONTROL_ON_GET_ELEMENT_ATTRIBUTES_REQUEST: {
        avrcp_element_attr_val_t* attrs = NULL;
        attrs = (avrcp_element_attr_val_t*)malloc(sizeof(avrcp_element_attr_val_t) * packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.attrs_count);
        for (int i = 0; i < packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.attrs_count; i++) {
            attrs[i].attr_id = packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.types[i];
            attrs[i].chr_set = packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.chr_sets[i];
            switch (attrs[i].attr_id) {
            case AVRCP_ATTR_TITLE:
                attrs[i].text = packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.title;
                break;
            case AVRCP_ATTR_ARTIST_NAME:
                attrs[i].text = packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.values.artist;
                break;
            case AVRCP_ATTR_ALBUM_NAME:
                attrs[i].text = NULL;
                break;
            case AVRCP_ATTR_TRACK_NUMBER:
                attrs[i].text = NULL;
                break;
            case AVRCP_ATTR_TOTAL_NUMBER_OF_TRACKS:
                attrs[i].text = NULL;
                break;
            case AVRCP_ATTR_GENRE:
                attrs[i].text = NULL;
                break;
            case AVRCP_ATTR_PLAYING_TIME_MS:
                attrs[i].text = NULL;
                break;
            case AVRCP_ATTR_COVER_ART_HANDLE:
                attrs[i].text = NULL;
                break;
            default:
                break;
            }
        }
        CALLBACK_FOREACH(CBLIST, avrcp_control_callbacks_t,
            get_element_attribute_cb,
            &packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.addr,
            packet->avrcp_control_cb._bt_avrcp_control_get_element_attribute.attrs_count,
            attrs);

        free(attrs);
    }

    break;

    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
