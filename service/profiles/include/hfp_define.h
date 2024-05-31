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
#ifndef __HFP_DEFINE_H__
#define __HFP_DEFINE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>

/* * HFP HF supported features - bit mask */
#define HFP_BRSF_HF_NREC 0x00000001 /* * 0, EC and/or NR function */
#define HFP_BRSF_HF_3WAYCALL 0x00000002 /* * 1, Call waiting and 3-way calling */
#define HFP_BRSF_HF_CLIP 0x00000004 /* * 2, CLI presentation capability */
#define HFP_BRSF_HF_BVRA 0x00000008 /* * 3, Voice recognition activation */
#define HFP_BRSF_HF_RMTVOLCTRL 0x00000010 /* * 4, Remote volume control */
#define HFP_BRSF_HF_ENHANCED_CALLSTATUS 0x00000020 /* * 5, Enhanced call status */
#define HFP_BRSF_HF_ENHANCED_CALLCONTROL 0x00000040 /* * 6, Enhanced call control */
#define HFP_BRSF_HF_CODEC_NEGOTIATION 0x00000080 /* * 7, Codec negotiation */
#define HFP_BRSF_HF_HFINDICATORS 0x00000100 /* * 8, HF Indicators */
#define HFP_BRSF_HF_ESCO_S4T2_SETTING 0x00000200 /* * 9, eSCO S4 (and T2) settings supported */

/* * HFP AG supported features - bit mask */
#define HFP_BRSF_AG_3WAYCALL 0x00000001 /* * 0, Three-way calling */
#define HFP_BRSF_AG_NREC 0x00000002 /* * 1, EC and/or NR function */
#define HFP_BRSF_AG_BVRA 0x00000004 /* * 2, Voice recognition function */
#define HFP_BRSF_AG_INBANDRING 0x00000008 /* * 3, In-band ring tone capability */
#define HFP_BRSF_AG_BINP 0x00000010 /* * 4, Attach a number to a voice tag */
#define HFP_BRSF_AG_REJECT_CALL 0x00000020 /* * 5, Ability to reject a call */
#define HFP_BRSF_AG_ENHANCED_CALLSTATUS 0x00000040 /* * 6, Enhanced call status */
#define HFP_BRSF_AG_ENHANCED_CALLCONTROL 0x00000080 /* * 7, Enhanced call control */
#define HFP_BRSF_AG_EXTENDED_ERRORRESULT 0x00000100 /* * 8, Extended Error Result Codes */
#define HFP_BRSF_AG_CODEC_NEGOTIATION 0x00000200 /* * 9, Codec negotiation */
#define HFP_BRSF_AG_HFINDICATORS 0x00000400 /* * 10, HF Indicators */
#define HFP_BRSF_AG_eSCO_S4T2_SETTING 0x00000800 /* * 11, eSCO S4 (and T2) settings supported */

/* * HFP AG proprietary features[31:16] - bit mask */
#define HFP_FEAT_AG_UNKNOWN_AT_CMD 0x00020000 /* Pass unknown AT commands to app */

typedef enum {
    HFP_IN_BAND_RINGTONE_NOT_PROVIDED = 0,
    HFP_IN_BAND_RINGTONE_PROVIDED,
} hfp_in_band_ring_state_t;

typedef enum {
    HFP_CODEC_UNKONWN, /* init state */
    HFP_CODEC_MSBC,
    HFP_CODEC_CVSD
} hfp_codec_type_t;

typedef struct {
    uint32_t sample_rate;
    uint8_t codec;
    uint8_t bit_width;
    uint8_t reserved1;
    uint8_t reserved2;
} hfp_codec_config_t;

typedef enum {
    HFP_ATCMD_CODE_ATA = 0x01,
    HFP_ATCMD_CODE_ATD = 0x02,
    HFP_ATCMD_CODE_BLDN = 0x1A,
    HFP_ATCMD_CODE_UNKNOWN = 0xFFFF,
} hfp_atcmd_code_t;

typedef enum {
    HFP_ATCMD_RESULT_OK, /* OK received */
    HFP_ATCMD_RESULT_TIMEOUT, /* Timeout before receiving any result code */
    HFP_ATCMD_RESULT_ERROR, /* ERROR received */
    HFP_ATCMD_RESULT_NOCARRIER, /* NO CARRIER received */
    HFP_ATCMD_RESULT_BUSY, /* BUSY received */
    HFP_ATCMD_RESULT_NOANSWER, /* NO ANSWER received */
    HFP_ATCMD_RESULT_DELAYED, /* DELAYED received */
    HFP_ATCMD_RESULT_BLACKLISTED, /* BLACKLISTED received */

    HFP_ATCMD_RESULT_CMEERR = 10, /* Start of CME ERROR code */
    HFP_ATCMD_RESULT_CMEERR_AGFAILURE = HFP_ATCMD_RESULT_CMEERR, /* CME ERROR: 0 - AG failure */
    HFP_ATCMD_RESULT_CMEERR_NOCONN2PHONE, /* CME ERROR: 1 - No connection to phone */
    HFP_ATCMD_RESULT_CMEERR_OPERATION_NOTALLOWED,
    HFP_ATCMD_RESULT_CMEERR_OPERATION_NOTSUPPORTED,
    HFP_ATCMD_RESULT_CMEERR_PHSIMPIN_REQUIRED,

    HFP_ATCMD_RESULT_CMEERR_SIMNOT_INSERTED = HFP_ATCMD_RESULT_CMEERR + 10, /* CME ERROR: 10 - SIM not inserted */
    HFP_ATCMD_RESULT_CMEERR_SIMPIN_REQUIRED,
    HFP_ATCMD_RESULT_CMEERR_SIMPUK_REQUIRED,
    HFP_ATCMD_RESULT_CMEERR_SIM_FAILURE,
    HFP_ATCMD_RESULT_CMEERR_SIM_BUSY,

    HFP_ATCMD_RESULT_CMEERR_INCORRECT_PASSWORD = HFP_ATCMD_RESULT_CMEERR + 16, /* CME ERROR: 16 - Incorrect password */
    HFP_ATCMD_RESULT_CMEERR_SIMPIN2_REQUIRED,
    HFP_ATCMD_RESULT_CMEERR_SIMPUK2_REQUIRED,

    HFP_ATCMD_RESULT_CMEERR_MEMORY_FULL = HFP_ATCMD_RESULT_CMEERR + 20, /* CME ERROR: 10 - Memory full */
    HFP_ATCMD_RESULT_CMEERR_INVALID_INDEX,

    HFP_ATCMD_RESULT_CMEERR_MEMORY_FAILURE = HFP_ATCMD_RESULT_CMEERR + 23, /* CME ERROR: 10 - Memory failure */
    HFP_ATCMD_RESULT_CMEERR_TEXTSTRING_TOOLONG,
    HFP_ATCMD_RESULT_CMEERR_INVALID_CHARACTERS_INTEXTSTRING,
    HFP_ATCMD_RESULT_CMEERR_DIAL_STRING_TOOLONG,
    HFP_ATCMD_RESULT_CMEERR_INVALID_CHARACTERS_INDIALSTRING,

    HFP_ATCMD_RESULT_CMEERR_NETWORK_NOSERVICE = HFP_ATCMD_RESULT_CMEERR + 30, /* CME ERROR: 10 - No network service */
    HFP_ATCMD_RESULT_CMEERR_NETWORK_TIMEOUT,
    HFP_ATCMD_RESULT_CMEERR_NETWORK_NOTALLOWED_EMERGENCYCALL_ONLY,

    /* The other CME error codes */

} hfp_atcmd_result_t;

#endif /* __HFP_DEFINE_H__ */