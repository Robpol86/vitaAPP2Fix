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

#ifndef SCE_CONST_H
#define SCE_CONST_H

/**
 * Bluetooth device name max size. Must match ksceBtGetDeviceName(name) size.
 */
#define VQMBT_SCE_DEVICE_NAME_MAX 0x79

/**
 * Device state returned by ksceBtGetConnectingInfo().
 */
typedef enum VqmbtInferredDevState : int {
    VQMBT_SCE_BT_STATE_UNKNOWN0 = 0,
    VQMBT_SCE_BT_STATE_DISCONNECTED = 1,
    VQMBT_SCE_BT_STATE_CONNECTING = 2,
    VQMBT_SCE_BT_STATE_CONNECTED_HID = 3,
    VQMBT_SCE_BT_STATE_DISCONNECTING = 4,
    VQMBT_SCE_BT_STATE_CONNECTED = 5,
    VQMBT_SCE_BT_STATE_REGISTERING = 6,
} VqmbtInferredDevState;

#endif  // SCE_CONST_H
