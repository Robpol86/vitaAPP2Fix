/*
This file is part of vitaAPP2Fix.
Copyright © 2026 Robpol86

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

/******************************************************************************
 * @file
 * @brief SCE constants inferred/guessed through trial and error.
 *
 * These values attempt to match Sony's undocumented interfaces. They must not
 * change unless the Vita SDK changes them or new reverse engineering efforts
 * uncover better alignments.
 ******************************************************************************/

#ifndef SCE_CONST_H_MODULE
#define SCE_CONST_H_MODULE

#include <psp2kern/bt.h>

/**
 * Bluetooth device name max size. Must match ksceBtGetDeviceName(name) size.
 */
#define VAPPTF_SCE_DEVICE_NAME_MAX 0x79

/**
 * SceBtEvent event IDs and their inferred meanings.
 */
typedef enum VapptfInferredBtEventId : unsigned char {
    VAPPTF_SCE_BT_EVENT_INQUIRY_RESULT = 0x01,
    VAPPTF_SCE_BT_EVENT_INQUIRY_STOP = 0x02,
    VAPPTF_SCE_BT_EVENT_PAIRING_REQUEST = 0x04,
    VAPPTF_SCE_BT_EVENT_CONNECT_RESULT = 0x05,
    VAPPTF_SCE_BT_EVENT_DISCONNECT = 0x06,
    VAPPTF_SCE_BT_EVENT_ADD_REMOVE_CONNECTING_DEVICE = 0x07,
    VAPPTF_SCE_BT_EVENT_CONNECT_REQUESTED = 0x08,
    VAPPTF_SCE_BT_EVENT_CONNECT_UNPAIRED = 0x09,
    VAPPTF_SCE_BT_EVENT_UNKNOWN0A = 0x0A,
    VAPPTF_SCE_BT_EVENT_UNKNOWN0B = 0x0B,
    VAPPTF_SCE_BT_EVENT_UNKNOWN0C = 0x0C,
    VAPPTF_SCE_BT_EVENT_BUTTON_PRESSED = 0x0D,
    VAPPTF_SCE_BT_EVENT_UNKNOWN0E = 0x0E,
    VAPPTF_SCE_BT_EVENT_UNKNOWN10 = 0x10,
    VAPPTF_SCE_BT_EVENT_UNKNOWN11 = 0x11,
    VAPPTF_SCE_BT_EVENT_TOGGLE_BLUETOOTH = 0x15,
    VAPPTF_SCE_BT_EVENT_UNKNOWN1C = 0x1C,
} VapptfInferredBtEventId;
static_assert(sizeof(VapptfInferredBtEventId) == sizeof(((SceBtEvent*)0)->id), "SceBtEvent.id changed size?");

#endif  // SCE_CONST_H_MODULE
