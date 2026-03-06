/*
 * Copyright (C) 2025 Xiaomi Corporation. All rights reserved.
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
 *
 */

#ifndef _CS_RAS_H_
#define _CS_RAS_H_

#include "service_loop.h"
#include "cs_ras_util.h"
#include "bt_addr.h"
#include "cs_ras_gatts.h"
#include "bt_gatt_defs.h"
#include "cs_ras_test.h"
#include "cs_service.h"

#ifndef BIT
#define BIT(n) (1UL << n)
#endif /* BIT */

/**
 * @brief Response timeout for Ranging Service (RAS) operations.
 *
 * This macro defines the timeout duration for Ranging Service (RAS)
 * responses, set to 5 seconds.
 */
#define RAS_RSP_TIMEOUT (5 * 1000)

/**
 * @brief Empty array indicator.
 *
 * This macro represents an empty array or zero-length data structure
 * used in Ranging Service contexts.
 */
#define RAS_EMPTY_ARRAY (0)

/**
 * @brief RAS role: Initiator.
 *
 * This macro defines the Ranging Service (RAS) role as the Initiator,
 * which starts the ranging process by sending ranging requests.
 */
#define CS_RAS_ROLE_INITIATOR (0)

/**
 * @brief RAS role: Reflector.
 *
 * This macro defines the Ranging Service (RAS) role as the Reflector,
 * which responds to ranging requests from the Initiator.
 */
#define CS_RAS_ROLE_REFLECTOR (1)

/**
 * @brief RAS subevent step mode 0.
 *
 * This macro defines subevent step mode 0 for the Ranging Service (RAS).
 * The meaning of this mode depends on the specific RAS configuration or
 * ranging algorithm being used.
 */
#define CS_RAS_SUBEVENT_STEP_MODE_0 (0)

/**
 * @brief RAS subevent step mode 1.
 *
 * This macro defines subevent step mode 1 for the Ranging Service (RAS).
 */
#define CS_RAS_SUBEVENT_STEP_MODE_1 (1)

/**
 * @brief RAS subevent step mode 2.
 *
 * This macro defines subevent step mode 2 for the Ranging Service (RAS).
 */
#define CS_RAS_SUBEVENT_STEP_MODE_2 (2)

/**
 * @brief RAS subevent step mode 3.
 *
 * This macro defines subevent step mode 3 for the Ranging Service (RAS).
 */
#define CS_RAS_SUBEVENT_STEP_MODE_3 (3)

/**
 * @brief GATT notification type for Ranging Service (RAS).
 *
 * This macro defines the use of GATT Notification in the Ranging Service (RAS)
 * for transmitting ranging-related data or events to a connected peer device.
 * Notifications are sent without requiring acknowledgment from the receiver.
 */
#define CS_RAS_GATT_NOTIFY (1)

/**
 * @brief GATT indication type for Ranging Service (RAS).
 *
 * This macro defines the use of GATT Indication in the Ranging Service (RAS)
 * for transmitting ranging-related data or events to a connected peer device.
 * Indications require acknowledgment (confirmation) from the receiver.
 */
#define CS_RAS_GATT_INDICATION (2)

#define CS_RAS_CONTROL_POINT_DATA_LEN    (3)
#define CS_RAS_CTL_OP_RSP_CODE_DATA_LEN  (2)

/**
 * @brief RAS sub-procedure header identifier.
 *
 * This macro defines the identifier value (12) used for the Ranging Service (RAS)
 * sub-procedure header. It marks the beginning of the RAS sub-procedure data block.
 *
 * The RAS Sub-Procedure Header includes the following fields:
 *
 * | Field                     | Size (bits) | Description                                                                                                              |
 * |----------------------------|-------------|--------------------------------------------------------------------------------------------------------------------------|
 * | Ranging Counter        | 12          | Lower 12 bits of `CS_Procedure_Counter` (see *Vol 4, Part E, Sec 7.7.65.44* in [1]) provided by the Core Controller.     |
 * | Configuration ID       | 4           | CS configuration identifier. Range: 0–3.                                                                                |
 * | Selected TX Power      | 8           | Transmit power level used for the CS Procedure. Range: -127 to 20 dBm (referenced to 1 mW).                              |
 * | Antenna Paths Mask     | 8           | Indicates which antenna paths are reported: <br>• Bit0: Path 1 <br>• Bit1: Path 2 <br>• Bit2: 
 *                                          Path 3 <br>• Bit3: Path 4 <br>• Bits 4–7: RFU |
 * | Subevent Header        | —           | —                                                                                                                        |
 * | Start ACL Connection Event | 16      | Starting ACL connection event count for results reported in the event.                                                   |
 * | Frequency Compensation | 16          | Frequency compensation value in units of 0.01 ppm (15-bit signed integer).                                              |
 * | Ranging Done Status    | 4           | Completion state for the CS Procedure: <br>• 0x0 – All results complete <br>• 0x1 – Partial results, more to 
 *                                          follow <br>• 0xF – All subsequent CS Procedures aborted <br>• Others – RFU |
 * | Subevent Done Status   | 4           | Completion state for the CS Subevent: <br>• 0x0 – All results complete <br>• 0xF – Subevent aborted <br>• Others – RFU |
 * | Ranging Abort Reason   | 4           | Abort reason when `Procedure_Done_Status` = 0xF; otherwise 0. <br>• 0x0 – No abort <br>• 0x1 – Local/remote abort 
 *                                          request <br>• 0x2 – Filtered channel map < 15 channels <br>• 0x3 – Channel map update instant passed <br>• 0xF – Unspecified <br>• Others – RFU |
 * | Subevent Abort Reason  | 4           | Abort reason when `Subevent_Done_Status` = 0xF; otherwise 0. <br>• 0x0 – No abort <br>• 0x1 – Local/remote abort request <br>• 0x2 – 
 *                                          No CS_SYNC (mode 0) received <br>• 0x3 – Scheduling/resource conflict <br>• 0xF – Unspecified <br>• Others – RFU |
 * | Reference Power Level  | 8           | Reference power level. Range: –127 to 20 dBm.                                                                           |
 * | Number of Steps Reported | 8         | Number of steps in the CS Subevent for which results are reported. If aborted, can be set to 0.                           |
 *
 * @note Refer to Bluetooth Core Specification Volume 4, Part E, Section 7.7.65.44 for field definitions.
 */
#define CS_RAS_SUB_PROCUDURE_HEAD (12)

/**
 * @brief CCCD "Not Implemented Configuration" error code for RAS.
 *
 * This macro defines the error code (0xFD) used in the Ranging Service (RAS)
 * to indicate that the Client Characteristic Configuration Descriptor (CCCD)
 * write request refers to a configuration that is not implemented or supported.
 *
 * Typically returned when a peer attempts to enable notifications or indications
 * on a characteristic that does not support the requested configuration.
 */
#define CS_RAS_CCCD_NOT_IMPR_CONFIG_ERR (0xFD)

/**
 * @brief CCCD write request rejected error code for RAS.
 *
 * This macro defines the error code (0xFC) used in the Ranging Service (RAS)
 * to indicate that the Client Characteristic Configuration Descriptor (CCCD)
 * write request was explicitly rejected by the server.
 *
 * This may occur due to invalid parameters, insufficient permissions,
 * or internal service state preventing configuration changes.
 */
#define CS_RAS_CCCD_WR_REQ_REJECT (0xFC)

/**
 * @brief Ranging mode definitions for the Ranging Service (RAS).
 *
 * RAS enables the client to read Ranging Data from a RAS Server. RAS is implemented
 * on a device that can generate Ranging Data using the Channel Sounding (CS) feature
 * of the local Core Controller. RAS distinguishes between two modes of Ranging Data exchange,
 * each represented by a service characteristic:
 *
 * • Real-time Ranging Data: Data received from the local Core Controller and
 *   communicated immediately by the RAS Server while connected to the client.
 * • On-demand Ranging Data: Data received from the local Core Controller and
 *   stored on the RAS Server for on-demand retrieval by the client.
 *
 * The client enables one of these modes by subscribing to either notifications or
 * indications of the corresponding characteristic (Real-time or On-demand) via the
 * Client Characteristic Configuration Descriptor (CCCD). The RAS Server shall operate
 * in either Real-time or On-demand mode, but **not both simultaneously**. A client can
 * switch modes by first disabling the active mode and then enabling the other mode.
 *
 * If a client attempts to enable more than one mode, the RAS Server shall reject the
 * Write Characteristic Descriptor request and return a `CS_RAS_CCCD_NOT_IMPR_CONFIG_ERR`
 * (0xFD). If the RAS Server does not support notifications for a characteristic and
 * the client enables notifications, the server shall reject the request and return
 * `CS_RAS_CCCD_WR_REQ_REJECT` (0xFC).
 *
 * A RAS Server shall transfer Ranging Data in chronological order, with the oldest
 * data sent first. This applies to both On-demand Ranging Data (transferred via
 * RAS Control Point procedure) and Real-time Ranging Data. If the ACL connection with
 * the client is lost or terminated, any pending Ranging Data segments shall be flushed.
 *
 * The RAS Ranging Modes are defined as follows:
 *
 * | **Macro**                             | **Value**  | **Description**                                                                 |
 * |---------------------------------------|------------|---------------------------------------------------------------------------------|
 * | **CS_RAS_RANGING_MODE_REAL_TIME** | 0x01       | Real-time ranging mode. Measurements are performed continuously or periodically in real time. |
 * | **CS_RAS_RANGING_MODE_ON_DEMAND** | 0x02       | On-demand ranging mode. Measurements are performed only when explicitly requested by the client. |
 * | **CS_RAS_RANGING_MODE_UNDEFINED** | 0xFF       | Undefined or invalid mode. Used as default or error value.                       |
 *
 * @note These values are typically exchanged via GATT characteristics as part of
 *       RAS control or configuration procedures.
 */
#define CS_RAS_RANGING_MODE_REAL_TIME (0x01)
#define CS_RAS_RANGING_MODE_ON_DEMAND (0x02)
#define CS_RAS_RANGING_MODE_UNDEFINED (0xFF)
typedef uint8_t ras_rang_mode_t;

/**
 * @brief Maximum number of Ranging Service (RAS) procedures that can be stored.
 *
 * This macro defines the upper limit on the number of RAS procedures that the
 * RAS Server can store simultaneously. Each stored procedure may contain
 * multiple steps or subevents of Ranging Data generated via the Channel Sounding (CS)
 * feature of the local Core Controller.
 *
 * The value 10 indicates that the server can hold up to 10 procedures in memory.
 * If additional procedures are generated beyond this limit, the server may
 * reject new procedures or overwrite older ones depending on the implementation.
 *
 * @note This value helps the RAS Server manage memory and resources efficiently,
 *       preventing overflow when multiple Ranging procedures are active.
 */
#define CS_RAS_STORE_PROCEDURE_NUM_MAX (10)

/**
 * RAS Features format
 * The RAS Features characteristic bit formats are listed in
 * +--------+-----------------------------------------------+
 * | Bit(s) | Definition                                    |
 * +--------+-----------------------------------------------+
 * | 0      | Real-time Ranging Data                        |
 * | 1      | Retrieve Lost Ranging Data Segments           |
 * | 2      | Abort Operation                               |
 * | 3      | Filter Ranging Data                           |
 * | 4–31   | Reserved for Future Use (RFU)                 |
 * +--------+-----------------------------------------------+
 */
#define CS_RAS_REAL_TIME_RANG_DATA_SUPPROTED (1 << 0)
#define CS_RAS_RETRI_LOST_RANG_DATA_SEG (1 << 1)
#define CS_RAS_ABORT_OPRATION (1 << 2)
#define CS_RAS_FILTER_RANG_DATA (1 << 3)

/**
 * @brief RAS Control Point Operation Codes (Op Codes) and Parameters.
 *
 * The Ranging Service (RAS) Control Point allows clients to issue commands
 * to a RAS Server to control and retrieve Ranging Data. Each operation has
 * a defined Op Code and associated parameters. Requirements are summarized
 * in Tables 3.10 and 3.11 of the specification.
 *
 * | **Operation**                          | **Op Code** | **Requirement** | **Parameter #1**           | **Parameter #2**          | **Parameter #3**          |
 * |----------------------------------------|------------|----------------|----------------------------|---------------------------|---------------------------|
 * | **Get_Ranging_Data**                    | 0x00       | Mandatory (M)  | uint16 Ranging Counter     | –                         | –                         |
 * | **ACK_Ranging_Data**                    | 0x01       | Mandatory (M)  | uint16 Ranging Counter     | –                         | –                         |
 * | **Retrieve_Lost_Ranging_Data_Segments**| 0x02       | Optional (O)   | uint16 Ranging Counter     | uint8 First Segment Index | uint8 Last Segment Index  |
 * | **Abort_Operation**                     | 0x03       | Optional (O)   | No Parameter used          | –                         | –                         |
 * | **Set_Filter**                          | 0x04       | Optional (O)   | uint16 Filter Configuration:<br>• Bits 0-1: Mode<br>• Bits 2-15: Filter bit mask (see Section 3.3.2.4) | – | – |
 *
 * @note The client uses these Op Codes via the RAS Control Point characteristic
 *       to perform operations on the RAS Server. The server shall respond according
 *       to the Op Code requirements, returning results or acknowledgements as defined.
 */
#define CS_RAS_CTL_OP_CMD_GET_RANG_DATA (0x00)
#define CS_RAS_CTL_OP_CMD_ACK_RANG_DATA (0x01)
#define CS_RAS_CTL_OP_CMD_RETRIEVE_LOST_RANG_DATA_SEG (0x02)
#define CS_RAS_CTL_OP_CMD_ABORT_OPERATION (0x03)
#define CS_RAS_CTL_OP_CMD_SET_FILTER (0x04)

/**
 * @brief RAS Control Point Response Op Codes and Parameters.
 *
 * The Ranging Service (RAS) Control Point characteristic also defines responses
 * from the RAS Server to the client. Each response has an Op Code and associated
 * parameters. Requirements are summarized in Table 3.11 of the specification.
 *
 * | **Response**                                | **Op Code** | **Requirement** | **Parameter #1**           | **Parameter #2**          | **Parameter #3**          |
 * |-------------------------------------------- |------------|----------------|----------------------------|---------------------------|---------------------------|
 * | **Complete Ranging Data Response**          | 0x00       | Mandatory (M)  | uint16 Ranging Counter     | –                         | –                         |
 * | **Complete Lost Ranging Data Segment Response** | 0x01   | Conditional (C.1) | uint16 Ranging Counter | uint8 First Segment Index | uint8 Last Segment Index  |
 * | **Response Code**                           | 0x02       | Mandatory (M)  | Response Code Value        | –                         | –                         |
 *
 * @note C.1: If the Retrieve Lost Ranging Data Segments procedure is supported,
 *       then this Op Code is considered Mandatory.
 *
 * @note These Op Codes are used by the RAS Server to report the results of
 *       operations initiated by the client via the RAS Control Point characteristic.
 */
#define CS_RAS_CTL_OP_RSP_CMP_RANG_DATA (0x00)
#define CS_RAS_CTL_OP_RSP_CMP_LOST_RANG_DATA_SEG (0x01)
#define CS_RAS_CTL_OP_RSP_CODE (0x02)

/**
 * @brief RAS Control Point Response Code Values (for Op Code 0x02).
 *
 * These values are used by the RAS Server to indicate the result of an operation
 * requested by the client via the RAS Control Point characteristic.
 * Requirements are summarized in Table 3.12 of the specification.
 *
 * | **Response Code Value** | **Definition**           | **Description**                                                             |
 * |------------------------|-------------------------|-------------------------------------------------------------------------------|
 * | 0x00                   | Reserved for Future Use | N/A                                                                           |
 * | 0x01                   | Success                 | Normal response for a successful operation                                    |
 * | 0x02                   | Op Code Not Supported   | Normal response if an unsupported Op Code is received                         |
 * | 0x03                   | Invalid Parameter       | Normal response if the received parameter does not meet service requirements  |
 * | 0x04                   | Success/Persisted       | Normal response for a successful write operation where values are persisted   |
 * | 0x05                   | Abort Unsuccessful      | Normal response if a request for Abort is unsuccessful                        |
 * | 0x06                   | Procedure Not Completed | Normal response if unable to complete a procedure for any reason              |
 * | 0x07                   | Server Busy             | Normal response if the Server is still busy with other requests               |
 * | 0x08                   | No Records Found        | Normal response if the requested Ranging Counter is not found                 |
 * | 0x09-0xFF              | Reserved for Future Use | N/A                                                                           |
 *
 * @note These codes are specifically returned in response to Control Point
 *       operations with Op Code 0x02 (Response Code Op).
 */
#define CS_RAS_CTL_OP_RSP_CODE_RESERVED (0x00)
#define CS_RAS_CTL_OP_RSP_CODE_SUCCESS (0x01)
#define CS_RAS_CTL_OP_RSP_CODE_NOT_SUPPORTED (0x02)
#define CS_RAS_CTL_OP_RSP_CODE_INVALID_PARAMS (0x03)
#define CS_RAS_CTL_OP_RSP_CODE_PERSISTED (0x04)
#define CS_RAS_CTL_OP_RSP_CODE_ABORT (0x05)
#define CS_RAS_CTL_OP_RSP_CODE_PROCE_NOT_CMP (0x06)
#define CS_RAS_CTL_OP_RSP_CODE_SERVER_BUSY (0x07)
#define CS_RAS_CTL_OP_RSP_CODE_NO_RECORD_FOUND (0x08)

/**
 * @brief RAS Filter Mode and Filter Bit Masks.
 *
 * These macros define the maximum filter mode, and masks for extracting
 * the mode and filter bits from a 16-bit Filter Configuration field used
 * in the Ranging Service (RAS) Control Point.
 */

/** Maximum valid RAS filter mode value (0x00 to 0x03). */
#define CS_RAS_FILTER_MODE_MAX (0x04)

/** Mask for the Mode bits (bits 0-1) in the Filter Configuration field. */
#define CS_RAS_FILTER_MODE_MASK (0x03) // Mask for Mode bits (0-1)

/** Mask for the Filter bits (bits 2-15) in the Filter Configuration field. */
#define CS_RAS_FILTER_BIT_MASK (0xFFFC) // Mask for Filter bits (2-15)

/**
 * @brief Maximum buffer length for RAS Step Data.
 *
 * This macro defines the maximum length of the buffer used to store
 * step data for the Ranging Service (RAS) GATT characteristic.
 *
 * @note The length is set according to the maximum GATT characteristic
 *       length supported by the Bluetooth stack.
 */
#define CS_RAS_STEP_DATA_BUF_LEN 2048


/**
 * @brief Copy a field to the output buffer if the corresponding filter bit is enabled.
 *
 * This macro conditionally copies a field of data from a source pointer `p`
 * to an output buffer `buf` depending on the `filter_mask`. It also updates
 * offsets and tracks remaining bytes.
 *
 * @param _size        Size of the field in bytes.
 * @param _filter_bit  Bitmask corresponding to the field in the filter_mask.
 *
 * @details
 * - The field is only copied if:
 *   1. There are enough bytes remaining (`remaining >= _size`), and
 *   2. The corresponding bit in `filter_mask` is set (`filter_mask & _filter_bit`).
 * - After copying, the output offset (`out_offset`) is incremented by `_size`.
 * - Regardless of copying, the source pointer `p` is advanced by `_size`,
 *   and `remaining` bytes are decreased by `_size`.
 * - If there are not enough bytes remaining to copy the field, a warning is logged
 *   and `remaining` is set to 0 to prevent further copying.
 *
 * @note This macro is typically used when serializing data fields conditionally
 *       based on a filter mask in Ranging Service (RAS) or similar protocols.
 */
#define COPY_FIELD_IF_ENABLED(_size, _filter_bit)              \
    do {                                                       \
        if (remaining >= (_size)) {                            \
            if (filter_mask & (_filter_bit)) {                 \
                memcpy(&buf[out_offset], p, (_size));          \
                out_offset += (_size);                         \
            }                                                  \
            p += (_size);                                      \
            remaining -= (_size);                              \
        } else {                                               \
            BT_LOGW("Field truncated, size=%d", (int)(_size)); \
            remaining = 0;                                     \
        }                                                      \
    } while (0)

/**
 * @brief Bits used to enable notifications or indications for RAS characteristics.
 *
 * This enumeration defines the bit positions for enabling notifications or
 * indications for various Ranging Service (RAS) GATT characteristics.
 * These bits are typically used in a client configuration mask to control
 * which types of RAS data are sent from the server to the client.
 */
enum cs_ras_notify_enable_bit {
    /** Enable notification for Real-time Ranging (RTT) Data characteristic. */
    RAS_RTT_DATA_NOTIFY,
    /** Enable indication for Real-time Ranging (RTT) Data characteristic. */
    RAS_RTT_DATA_INDICATE,
    /** Enable notification for RAS Control Point characteristic. */
    RAS_CONTROL_POINT_NOTIFY,
    /** Enable indication for RAS Control Point characteristic. */
    RAS_CONTROL_POINT_INDICATE,
    /** Enable notification for On-demand Ranging Data characteristic. */
    RAS_ON_DEMAND_DATA_NOTIFY,
    /** Enable indication for On-demand Ranging Data characteristic. */
    RAS_ON_DEMAND_DATA_INDICATE,
    /** Enable notification for Data Ready characteristic. */
    RAS_DATA_READY_NOTIFY,
    /** Enable indication for Data Ready characteristic. */
    RAS_DATA_READY_INDICATE,
    /** Enable notification for Overwrite event characteristic. */
    RAS_OVER_WRITE_NOTIFY,
    /** Enable indication for Overwrite event characteristic. */
    RAS_OVER_WRITE_INDICATE
};

/**
 * @brief RAS GATT Attribute Table Indexes.
 *
 * This enumeration defines indexes for the Ranging Service (RAS) GATT attributes.
 * These indexes are used to reference characteristics, their values, and
 * Client Characteristic Configuration Descriptors (CCCDs) in the attribute table.
 */
enum {
    /** RAS Service Index. */
    CS_RAS_SVC_IDX,

    /** Feature Characteristic Index. */
    CS_RAS_FEAT_CHAR_IDX,
    /** Feature Characteristic Value Index. */
    CS_RAS_FEAT_CHAR_VAL_IDX,

    /** Real-time Data Characteristic Index. */
    CS_RAS_RT_DT_CHAR_IDX,
    /** Real-time Data Characteristic Value Index. */
    CS_RAS_RT_DT_CHAR_VAL_IDX,
    /** Real-time Data Characteristic CCC Descriptor Index. */
    CS_RAS_RT_DT_CCC_CFG_IDX,

    /** On-demand Data Characteristic Index. */
    CS_RAS_ON_DEM_CHAR_IDX,
    /** On-demand Data Characteristic Value Index. */
    CS_RAS_ON_DEM_CHAR_VAL_IDX,
    /** On-demand Data Characteristic CCC Descriptor Index. */
    CS_RAS_ON_DEM_CCC_CFG_IDX,

    /** Control Point Characteristic Index. */
    CS_RAS_CTR_PT_CHAR_IDX,
    /** Control Point Characteristic Value Index. */
    CS_RAS_CTR_PT_CHAR_VAL_IDX,
    /** Control Point Characteristic CCC Descriptor Index. */
    CS_RAS_CTR_PT_CCC_CFG_IDX,

    /** Data Ready Characteristic Index. */
    CS_RAS_DT_RD_CHAR_IDX,
    /** Data Ready Characteristic Value Index. */
    CS_RAS_DT_RD_CHAR_VAL_IDX,
    /** Data Ready Characteristic CCC Descriptor Index. */
    CS_RAS_DT_RD_CCC_CFG_IDX,

    /** Data Overwrite Characteristic Index. */
    CS_RAS_DT_OV_WR_CHAR_IDX,
    /** Data Overwrite Characteristic Value Index. */
    CS_RAS_DT_OV_WR_CHAR_VAL_IDX,
    /** Data Overwrite Characteristic CCC Descriptor Index. */
    CS_RAS_DT_OV_WR_CCC_CFG_IDX,

    /** Total number of RAS attribute table entries. */
    CS_RAS_IDX_MAX
};

/*******************************************************************************************
 *
 *  FILTER BIT MAPPING TABLE
 *  ------------------------------------------------------------
 *  Each field can be individually filtered out using the
 *  uint32_t ras_filter[CS_RAS_FILTER_MODE_MAX] bitmask.
 *
 *  +----------------------------+-------------------------------+
 *  | Field Name                 | Bit Definition                |
 *  +----------------------------+-------------------------------+
 *  | Packet_Quality             | CS_RAS_FILTER_BIT_PKT_QUALITY     (BIT(0))  |
 *  | Packet_NADM                | CS_RAS_FILTER_BIT_PKT_NADM        (BIT(1))  |
 *  | Packet_RSSI                | CS_RAS_FILTER_BIT_PKT_RSSI        (BIT(2))  |
 *  | Packet_Antenna             | CS_RAS_FILTER_BIT_PKT_ANTENNA     (BIT(3))  |
 *  | Packet_PCT1                | CS_RAS_FILTER_BIT_PKT_PCT1        (BIT(4))  |
 *  | Packet_PCT2                | CS_RAS_FILTER_BIT_PKT_PCT2        (BIT(5))  |
 *  | Measured_Freq_Offset       | CS_RAS_FILTER_BIT_FREQ_OFFSET     (BIT(6))  |
 *  | ToA_ToD_Initiator          | CS_RAS_FILTER_BIT_TOA_TOD         (BIT(7))  |
 *  | ToD_ToA_Reflector          | CS_RAS_FILTER_BIT_TOD_TOA         (BIT(8))  |
 *  | Antenna_Permutation_Index  | CS_RAS_FILTER_BIT_ANT_PERM_IDX    (BIT(9))  |
 *  | Tone_PCT[k]                | CS_RAS_FILTER_BIT_TONE_PCT        (BIT(10)) |
 *  | Tone_Quality_Indicator[k]  | CS_RAS_FILTER_BIT_TONE_QUALITY    (BIT(11)) |
 *  +----------------------------+-------------------------------+
 *
 *******************************************************************************************/
typedef enum {
    CS_RAS_FILTER_BIT_PKT_QUALITY = BIT(0),
    CS_RAS_FILTER_BIT_PKT_NADM = BIT(1),
    CS_RAS_FILTER_BIT_PKT_RSSI = BIT(2),
    CS_RAS_FILTER_BIT_PKT_ANTENNA = BIT(3),
    CS_RAS_FILTER_BIT_PKT_PCT1 = BIT(4),
    CS_RAS_FILTER_BIT_PKT_PCT2 = BIT(5),
    CS_RAS_FILTER_BIT_FREQ_OFFSET = BIT(6),
    CS_RAS_FILTER_BIT_TOA_TOD = BIT(7),
    CS_RAS_FILTER_BIT_TOD_TOA = BIT(8),
    CS_RAS_FILTER_BIT_ANT_PERM_IDX = BIT(9),
    CS_RAS_FILTER_BIT_TONE_PCT = BIT(10),
    CS_RAS_FILTER_BIT_TONE_QUALITY = BIT(11),
} ras_filter_bits_t;

/**
 * @brief RAS Mode 0 Filter Bit Definitions.
 *
 * These enumeration values define the filter bit positions for RAS Mode 0.
 * Each bit indicates whether a particular type of packet or measurement data
 * should be included in the Ranging Service procedure.
 */
enum {
    CS_RAS_MODE_0_FILTER_PACKET_QUALITY,
    CS_RAS_MODE_0_FILTER_PACKET_RSSI,
    CS_RAS_MODE_0_FILTER_PACKET_ANTENNA,
    CS_RAS_MODE_0_FILTER_MEASURED_FREQ_OFFSET,
    CS_RAS_MODE_0_FILTER_MAX,
};

/**
 * @brief RAS Mode 1 Filter Bit Definitions.
 *
 * Filter bit positions for RAS Mode 1, specifying which packet or measurement
 * fields are included in the Ranging Service procedure.
 */
enum {
    CS_RAS_MODE_1_FILTER_PACKET_QUALITY,
    CS_RAS_MODE_1_FILTER_PACKET_NADM,
    CS_RAS_MODE_1_FILTER_PACKET_RSSI,
    CS_RAS_MODE_1_FILTER_TOD_TOA,
    CS_RAS_MODE_1_FILTER_PACKET_ANTENNA,
    CS_RAS_MODE_1_FILTER_PACKET_PCT_1,
    CS_RAS_MODE_1_FILTER_PACKET_PCT_2,
    CS_RAS_MODE_1_FILTER_MAX,
};

/**
 * @brief RAS Mode 2 Filter Bit Definitions.
 *
 * Filter bit positions for RAS Mode 2, primarily used for antenna permutation
 * and tone analysis in the Ranging Service procedure.
 */
enum {
    CS_RAS_MODE_2_FILTER_ANTENNA_PERMUTATION_INDEX,
    CS_RAS_MODE_2_FILTER_TONE_PCT,
    CS_RAS_MODE_2_FILTER_TONE_QUALITY_INDICATOR,
    CS_RAS_MODE_2_FILTER_ANTENNA_PATH_1,
    CS_RAS_MODE_2_FILTER_ANTENNA_PATH_2,
    CS_RAS_MODE_2_FILTER_ANTENNA_PATH_3,
    CS_RAS_MODE_2_FILTER_ANTENNA_PATH_4,
    CS_RAS_MODE_2_FILTER_MAX,
};

/**
 * @brief RAS Mode 3 Filter Bit Definitions.
 *
 * Filter bit positions for RAS Mode 3. Combines Mode 1 and Mode 2 filters
 * to enable comprehensive packet and antenna/tone analysis.
 */
enum {
    CS_RAS_MODE_3_FILTER_PACKET_QUALITY,
    CS_RAS_MODE_3_FILTER_PACKET_NADM,
    CS_RAS_MODE_3_FILTER_PACKET_RSSI,
    CS_RAS_MODE_3_FILTER_TOD_TOA,
    CS_RAS_MODE_3_FILTER_PACKET_ANTENNA,
    CS_RAS_MODE_3_FILTER_PACKET_PCT_1,
    CS_RAS_MODE_3_FILTER_PACKET_PCT_2,
    CS_RAS_MODE_3_FILTER_ANTENNA_PERMUTATION_INDEX,
    CS_RAS_MODE_3_FILTER_TONE_PCT,
    CS_RAS_MODE_3_FILTER_TONE_QUALITY_INDICATOR,
    CS_RAS_MODE_3_FILTER_ANTENNA_PATH_1,
    CS_RAS_MODE_3_FILTER_ANTENNA_PATH_2,
    CS_RAS_MODE_3_FILTER_ANTENNA_PATH_3,
    CS_RAS_MODE_3_FILTER_ANTENNA_PATH_4,
    CS_RAS_MODE_3_FILTER_MAX,
};

/**
 * @brief On-demand RAS Service State Machine.
 *
 * These enumeration values represent the different states of the On-demand
 * Ranging Service procedure. The state machine controls the sequence of
 * data ready notifications, data retrieval, sending, acknowledgment, and
 * completion.
 */
enum {
    /** Idle state. No operation in progress. */
    CS_RAS_ON_DEMAND_STATE_IDLE,

    /** Data ready indication has been sent to client. */
    CS_RAS_ON_DEMAND_STATE_DATA_READY_INDICATE,

    /** Waiting for Get Ranging Data response from client. */
    CS_RAS_ON_DEMAND_STATE_GET_RANGING_DATA_RSP,

    /** Ranging Data is being sent to client. */
    CS_RAS_ON_DEMAND_STATE_RANGING_DATA_SENDING,

    /** All Ranging Data segments have been sent. */
    CS_RAS_ON_DEMAND_STATE_RANGING_DATA_COMPLETE_SEND,

    /** Waiting for acknowledgment from client for Ranging Data. */
    CS_RAS_ON_DEMAND_STATE_RANGING_DATA_ACK_RECV,

    /** Waiting for Ranging Data response. */
    CS_RAS_ON_DEMAND_STATE_RANGING_DATA_RSP,

    /** Retrieving lost Ranging Data segments received from client. */
    CS_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_RECV,

    /** Lost Ranging Data segments response being sent. */
    CS_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_RSP,

    /** Completion of retrieving lost Ranging Data segments. */
    CS_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_COMPLETE,

    /** Waiting for acknowledgment of lost Ranging Data segments. */
    CS_RAS_ON_DEMAND_STATE_RETRIEVE_LOST_RANGING_DATA_ACK,

    /** Number of On-demand states. */
    CS_RAS_ON_DEMAND_STATE_NUMS,
};

/**
 * @brief RAS Control Point Operation Codes.
 *
 * Enumeration of possible opcodes for RAS control point operations.
 */
typedef enum {
    ABORT_OPERATION = 0, /**< Abort the ongoing Ranging operation. */
    OTHER_OPERATION = 1  /**< Placeholder for other operations. */
} ras_opcode_t;

/**
 * @brief RAS Data Segment Structure.
 *
 * Represents a single segment of Ranging Data for On-demand transfers.
 */
typedef struct ras_segment_t {
    cs_node_t seg_node;   /**< Node for linked list of segments. */
    uint16_t seg_idx;          /**< Segment index in the sequence. */
    uint16_t len;              /**< Length of the segment data. */
    uint8_t data[RAS_EMPTY_ARRAY]; /**< Flexible array member for segment payload. */
} ras_segment_t;

/**
 * @brief RAS On-demand Ranging Data Structure.
 *
 * Tracks the state of On-demand Ranging Data procedure for a client.
 */
typedef struct ras_rang_on_demand_t {
    bool proc_used;                /**< Indicates if this procedure slot is in use. */
    uint16_t count;                /**< Count of segments or data items. */
    service_timer_t* on_demand_timer;  /**< Timer/work item for response timeout. */
    cs_list_t seg_list;          /**< Linked list of Ranging Data segments. */
    ras_segment_t* seg;            /**< Pointer to the current segment being processed. */
} ras_rang_on_demand_t;

/**
 * @brief RAS Control Point Status.
 *
 * Tracks the current operation and processing state for the RAS Control Point.
 */
typedef struct {
    ras_opcode_t opcode;        /**< Current opcode being processed. */
    int is_processing;          /**< 1 if operation is in progress, 0 otherwise. */
    int is_data_pending;        /**< 1 if data is pending to send, 0 otherwise. */
    uint8_t response_code;      /**< Last response code sent to client. */
} ras_control_point_t;

/**
 * @brief RAS Service Environment.
 *
 * Maintains all runtime state and data for the RAS BLE Service instance.
 */
typedef struct {
    uint16_t step_data_attr_handle;          /**< GATT handle of Step Data characteristic. */
    bt_address_t* addr;                      /**< Current BLE address reference. */
    uint8_t latest_local_steps[CS_RAS_STEP_DATA_BUF_LEN]; /**< Buffer for step or ranging data. */
    uint8_t rt_dt_ccc_cfg;                   /**< CCC configuration for Real-time Data characteristic. */
    uint8_t ras_dt_rd_indicating;            /**< Flag indicating Ranging Data indication state. */
    uint8_t ras_role;                        /**< RAS role (Server/Client). */
    uint32_t ras_mtu;                        /**< Maximum Transfer Unit for RAS GATT operations. */
    uint32_t ras_seg_offset;                 /**< Offset in the current Ranging Data segment. */
    uint32_t remaining_len;                  /**< Remaining bytes to send in current operation. */
    uint8_t ras_seg_idx;                      /**< Index of the current Ranging Data segment. */
    uint32_t ras_feature;                     /**< Bitfield indicating RAS feature support. */
    uint32_t char_notify_state;               /**< Bitfield tracking characteristic notification/indication state. */
    uint32_t ras_filter[CS_RAS_FILTER_MODE_MAX]; /**< Filter settings per RAS mode (0-3). */
    uint32_t on_demand_state;                 /**< Current state of the On-demand RAS procedure. */
    ras_rang_on_demand_t subevent[CS_RAS_STORE_PROCEDURE_NUM_MAX]; /**< Array of On-demand procedure slots. */
    ras_control_point_t control_point;        /**< Control Point status for current operation. */
    cs_node_t* on_demand_curr_node;           /**< Pointer to current node in On-demand segment list. */
    uint16_t procedure_count;                  /**< Records the current CS procedure counter for On-demand Ranging Data. */
} ras_srv_env_t;

/**
 * @brief Write the step data for the CS Reflector.
 *
 * This function is used to send the latest ranging step data
 * to the reflector device.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int write_cs_reflector_step_data(void);

/**
 * @brief Enable the Channel Sounding (CS) feature.
 *
 * This function initializes and enables the local Core Controller
 * to generate Ranging Data for RAS (Ranging Service) operations.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int bt_cs_ras_enable(void);

/**
 * @brief Disable the Channel Sounding (CS) feature.
 *
 * This function Deinit and Disable the local Core Controller
 * to generate Ranging Data for RAS (Ranging Service) operations.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int bt_cs_ras_disable(void);

#ifdef CONFIG_BT_CS_RAS_TEST

/**
 * @brief Notify callback wrapper for test mode.
 *
 * Redirects the GATT notification callback to the test implementation.
 */
#define BT_GATT_NOTIFY_CB(attr, addr, value, len) bt_gatt_notify_cb_test(attr, addr, value, len)

/**
 * @brief Attribute read wrapper for test mode.
 *
 * Redirects the GATT attribute read operation to the test implementation.
 */
#define BT_GATT_ATTR_READ(addr, attr, buf, buf_len, offset, value, value_len) //bt_gatt_attr_read_test(addr, attr, buf, buf_len, offset, value, value_len)

/**
 * @brief GATT notify wrapper for test mode.
 *
 * Redirects the GATT notification operation to the test implementation.
 */
#define BT_GATT_NOTIFY(attr, addr, value, len) bt_gatt_notify_test(attr, addr, value, len)

/**
 * @brief GATT indicate wrapper for test mode.
 *
 * Redirects the GATT indication operation to the test implementation.
 */
#define BT_GATT_INDICATE(attr, addr, value, len) bt_gatt_indicate_test(attr, addr, value, len)

#else

/**
 * @brief Notify callback wrapper for normal mode.
 *
 * Uses the standard GATT notification callback.
 */
#define BT_GATT_NOTIFY_CB(attr, addr, value, len) ras_gatts_data_send_notify(attr, addr, value, len, true)

/**
 * @brief GATT notify wrapper for normal mode.
 *
 * Uses the standard GATT notification function.
 */
#define BT_GATT_NOTIFY(attr, addr, value, len) ras_gatts_data_send_notify(attr, addr, value, len, true)

/**
 * @brief GATT indicate wrapper for normal mode.
 *
 * Uses the standard GATT indication function.
 */
#define BT_GATT_INDICATE(attr, addr, value, len) ras_gatts_data_send_notify(attr, addr, value, len, false)

#endif /* CONFIG_BT_CS_RAS_TEST */

/**
 * @brief Test helper: simulate receiving a RAS subevent.
 *
 * This function is used for test purposes to inject a subevent
 * result into the RAS stack.
 *
 * @param mode The Ranging mode used for the test (e.g., Real-time or On-demand).
 * @param test_case The specific test case scenario to simulate.
 * @param addr Pointer to the bluetooth address associated with this subevent.
 * @param result Pointer to the subevent result data to inject.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ras_subevent_recv_test(ras_rang_mode_t mode, ras_testcase_t test_case,
    bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result);

/**
 * @brief Test helper: simulate sending a Control Point response.
 *
 * This function is used for testing the RAS Control Point procedure.
 *
 * @param addr Pointer to the bluetooth address to send the response to.
 * @param data Pointer to the data to send.
 * @param len Length of the data in bytes.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ras_ctrl_point_send_test(bt_address_t* addr, uint8_t* data, uint16_t len);

void ras_on_demand_notify_finish_test(bt_address_t* addr);

void ras_on_demand_indicate_finish_test(bt_address_t* addr, ras_attr_notify_t attr);
#endif /* _CS_RAS_H_ */