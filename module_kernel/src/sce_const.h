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
#define VAPPTF_SCE_BT_EVENT_LIST(X)       \
    X(INQUIRY_RESULT, 0x01)               \
    X(INQUIRY_STOP, 0x02)                 \
    X(PAIRING_REQUEST, 0x04)              \
    X(CONNECT_RESULT, 0x05)               \
    X(DISCONNECT, 0x06)                   \
    X(ADD_REMOVE_CONNECTING_DEVICE, 0x07) \
    X(CONNECT_REQUESTED, 0x08)            \
    X(CONNECT_UNPAIRED, 0x09)             \
    X(UNKNOWN0A, 0x0A)                    \
    X(UNKNOWN0B, 0x0B)                    \
    X(UNKNOWN0C, 0x0C)                    \
    X(BUTTON_PRESSED, 0x0D)               \
    X(UNKNOWN0E, 0x0E)                    \
    X(UNKNOWN10, 0x10)                    \
    X(UNKNOWN11, 0x11)                    \
    X(TOGGLE_BLUETOOTH, 0x15)             \
    X(UNKNOWN1C, 0x1C)
typedef enum VapptfInferredBtEventId : unsigned char {
#define X(name, val) VAPPTF_SCE_BT_EVENT_##name = (val),
    VAPPTF_SCE_BT_EVENT_LIST(X)
#undef X
} VapptfInferredBtEventId;
static_assert(sizeof(VapptfInferredBtEventId) == sizeof(((SceBtEvent*)0)->id), "SceBtEvent.id changed size?");

#endif  // SCE_CONST_H_MODULE
