/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#ifndef _BT_DFX_REASON_H_
#define _BT_DFX_REASON_H_

// error reason
#define BT_DFXE_REPEATED_ATTEMPT "btRepeatedAttempt"
#define BT_DFXE_ADAPTER_STATE_NOT_ON "btAdapterStateNotOn"
#define BT_DFXE_PAGE_TIMEOUT "btPageTimeout"
#define BT_DFXE_CONN_TIMEOUT "btConnTimeout"
#define BT_DFXE_CONN_FAILED_TO_BE_ESTABLISHED "btConnFailedToBeEstablished"

#define BT_DFXE_SCANNER_EXCEED_MAX_NUM "btScannerExceedMaxNum"

#define BT_DFXE_SPP_NOT_STARTUP "btSppNotStartup"
#define BT_DFXE_SPP_SCN_ALLOC_FAIL "btSppScnAllocFail"
#define BT_DFXE_SPP_NO_RESOURCES "btSppNoResources"

#define BT_DFXE_A2DP_CONN_TIMEOUT "btA2dpConnTimeout"
#define BT_DFXE_SET_A2DP_AVAILABLE_FAIL "btSetA2dpAvailableFail"
#define BT_DFXE_GET_A2DP_AVAILABLE_FAIL "btGetA2dpAvailableFail"

#define BT_DFXE_OFFLOAD_START_TIMEOUT "btOffloadStartTimeout"
#define BT_DFXE_OFFLOAD_HCI_UNSPECIFIED_ERROR "btOffloadHciUnspecifiedError"

#define BT_DFXE_GET_MEDIA_VOLUME_RANGE_FAIL "btGetMediaVolumeRangeFail"
#define BT_DFXE_SET_MEDIA_VOLUME_FAIL "btSetMediaVolumeFail"
#define BT_DFXE_SET_UI_VOLUME_FAIL "btSetUiVolumeFail"
#define BT_DFXE_MEDIA_PLAYER_CREATE_FAIL "btMediaPlayerCreateFail"
#define BT_DFXE_GET_STREAM_VOLUME_FAIL "btGetStreamVolumeFail"
#define BT_DFXE_MEDIA_SESSION_OPEN_FAIL "btMediaSessionOpenFail"
#define BT_DFXE_MEDIA_SESSION_SET_EVENT_CB_FAIL "btMediaSessionSetEventCbFail"
#define BT_DFXE_MEDIA_SESSION_START_FAIL "btMediaSessionStartFail"
#define BT_DFXE_MEDIA_SESSION_STOP_FAIL "btMediaSessionStopFail"
#define BT_DFXE_MEDIA_SESSION_PAUSE_FAIL "btMediaSessionPauseFail"
#define BT_DFXE_MEDIA_SESSION_NEXT_SONG_FAIL "btMediaSessionNextSongFail"
#define BT_DFXE_MEDIA_SESSION_PREV_SONG_FAIL "btMediaSessionPrevSongFail"

#define BT_DFXE_HFP_AG_CONN_TIMEOUT "btHfpAgConnTimeout"
#define BT_DFXE_HFP_AG_CONN_RETRY_FAIL "btHfpAgConnRetryFail"
#define BT_DFXE_HFP_HF_CONN_TIMEOUT "btHfpHfConnTimeout"
#define BT_DFXE_HFP_HF_CONN_RETRY_FAIL "btHfpHfConnRetryFail"
#define BT_DFXE_SET_VOICE_CALL_VOLUME_FAIL "btSetVoiceCallVolumeFail"
#define BT_DFXE_GET_VOICE_CALL_VOLUME_FAIL "btGetVoiceCallVolumeFail"
#define BT_DFXE_MEDIA_POLICY_SUBSCRIBE_FAIL "btMediaPolicySubscribeFail"
#define BT_DFXE_SET_HFP_SAMPLERATE_FAIL "btSetHfpSamplerateFail"
#define BT_DFXE_SET_SCO_AVAILABLE_FAIL "btSetScoAvailableFail"
#define BT_DFXE_SET_SCO_UNAVAILABLE_FAIL "btSetScoUnavailableFail"
#define BT_DFXE_SET_ANC_ENABLE_FAIL "btSetAncEnableFail"

#define BT_DFXE_HID_CONNECT_BUSY "btHidConnectBusy"

#define BT_DFXE_CLIENT_CONNECT_FAIL "btClientConnectFail"
#define BT_DFXE_ASYNC_CLIENT_CONN_FAIL "btAsyncClientConnectFail"
#define BT_DFXE_FILE_DESCRIPTOR_ERROR "btFileDescriptorError"
#define BT_DFXE_SPP_CONN_FAIL "btSppConnFail"
#define BT_DFXE_CLIENT_MSG_ALLOC_FAIL "btClientMsgAllocFail"
#define BT_DFXE_SERVER_CACHE_ALLOC_FAIL "btServerCacheAllocFail"
#define BT_DFXE_OPEN_HCI_UART_FAIL "btOpenHciUartFail"
#define BT_DFXE_LE_ENABLE_FAIL "btLeEnableFail"
#define BT_DFXE_BR_ENABLE_FAIL "btBrEnableFail"

#endif /* _BT_DFX_REASON_H_ */