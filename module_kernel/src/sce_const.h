/*
This file is part of vitaQmBluetooth.
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
 * SceBtEvent event IDs and their inferred meanings.
 */
typedef enum VqmbtInferredBtEventId : unsigned char {
    VQMBT_SCE_BT_EVENT_INQUIRY_RESULT = 0x01,
    VQMBT_SCE_BT_EVENT_INQUIRY_STOP = 0x02,
    VQMBT_SCE_BT_EVENT_PAIRING_REQUEST = 0x04,
    VQMBT_SCE_BT_EVENT_CONNECT_RESULT = 0x05,
    VQMBT_SCE_BT_EVENT_DISCONNECT = 0x06,
    VQMBT_SCE_BT_EVENT_ADD_REMOVE_CONNECTING_DEVICE = 0x07,
    VQMBT_SCE_BT_EVENT_CONNECT_REQUESTED = 0x08,
    VQMBT_SCE_BT_EVENT_CONNECT_UNPAIRED = 0x09,
    VQMBT_SCE_BT_EVENT_UNKNOWN0A = 0x0A,
    VQMBT_SCE_BT_EVENT_UNKNOWN0B = 0x0B,
    VQMBT_SCE_BT_EVENT_UNKNOWN0C = 0x0C,
    VQMBT_SCE_BT_EVENT_BUTTON_PRESSED = 0x0D,
    VQMBT_SCE_BT_EVENT_UNKNOWN0E = 0x0E,
    VQMBT_SCE_BT_EVENT_UNKNOWN10 = 0x10,
    VQMBT_SCE_BT_EVENT_UNKNOWN11 = 0x11,
    VQMBT_SCE_BT_EVENT_TOGGLE_BLUETOOTH = 0x15,
    VQMBT_SCE_BT_EVENT_UNKNOWN1C = 0x1C,
} VqmbtInferredBtEventId;
_Static_assert(sizeof(VqmbtInferredBtEventId) == sizeof(((SceBtEvent*)0)->id), "SceBtEvent.id changed size?");

#endif  // SCE_CONST_H_MODULE
