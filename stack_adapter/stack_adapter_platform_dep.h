/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*
* Copyright (c) Barrot Technology Limited
*
* All rights reserved.
*
---------------------------------------------------------------------------*/

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Module Name:
    stack_adapter_platform_dep.h
Abstract:
    Platform dependent header files, type, constants, routines.
    This is a sample implementation for test application only.
    It SHALL be rewritten according to the target platform.
Author:
    Y.D.X
---------------------------------------------------------------------------*/

#ifndef __STACK_ADAPTER_PLATFORM_DEP_H__
#define __STACK_ADAPTER_PLATFORM_DEP_H__

/*-----------------------------------------------------------------------------
Platform dependent includes Start
*/

/* This is for Windows PC test application */
#include <windows.h>

/* This is for debug output */
#include <string.h>
#include <time.h>
#include <assert.h>

/*
 Platform dependent includes End
-----------------------------------------------------------------------------*/


#define uint8_t     unsigned char
#define uint16_t    unsigned short
#define uint32_t    unsigned long
#define int8_t      signed char
#define int16_t     signed short
#define int32_t     signed long
#define bool        int

#define UNUSED(_v)
#define CONST_UNUSED(_v)

/************************************************************************************************
 *                                Platform dependent debug output functions
 ***********************************************************************************************/
/**
 * Write debug string to the application defined output device.
 * @param[in] dbg_str_length - total length, in bytes, of the dbg_str.
 * @param[in] dbg_str - debug string to be output
 * @return    void
 */
void platform_ext_debug_print(uint32_t dbg_str_length, uint8_t *dbg_str);

/**
 * HCI dump initialization.
 * @param   void.
 * @return  void
 */
void platform_ext_hci_dump_open(void);

/**
 * Write debug string to the application defined output device.
 * @param[in] ctrl_to_host - Packet from Controller to Host or not.
 * @param[in] hci_pkt - HCI raw packet starting with one byte packet type.
 * @param[in] hci_pkt_size - The size, in bytes, of the hci_pkt.
 * @return    void
 */
void platform_ext_hci_dump(uint8_t ctrl_to_host, uint8_t *hci_pkt, uint32_t hci_pkt_size);

/**
 * HCI dump terminates.
 * @param   void.
 * @return  void
 */
void platform_ext_hci_dump_close(void);

#endif // #ifndef __STACK_ADAPTER_PLATFORM_DEP_H__

