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
 * @brief Cross-module interface exposed by the kernel module.
 ******************************************************************************/

#ifndef VQMBT_H
#define VQMBT_H

#include <psp2common/types.h>
#include <stdbool.h>

#include "sce_const.h"

#define VQMBT_MAX_DEVICES 8  // Maximum number of bluetooth devices supported by the plugin.

// Error codes.
typedef enum VqmbtError : int {
    VQMBT_ERROR_INVALID_ARGUMENT = (int)0x80690001,  // Arbitrary first value.
    VQMBT_ERROR_KERNEL_SIDE,
    VQMBT_ERROR_KERNEL_SIDE_NOT_CONNECTABLE,
    VQMBT_ERROR_KERNEL_SIDE_BUSY,
    VQMBT_ERROR_CB_OVERFLOW,
    VQMBT_ERROR_NOT_READY,
    VQMBT_ERROR_GENERAL_FAILURE,
} VqmbtError;

// Device info.
typedef struct VqmbtDeviceInfo {
    char name[VQMBT_SCE_DEVICE_NAME_MAX];
    unsigned int mac0;
    unsigned int mac1;
    VqmbtInferredDevState state;
} VqmbtDeviceInfo;

// Inter-module events.
typedef enum VqmbtEventId : unsigned int {
    VQMBT_EVENT_NONE,
    VQMBT_EVENT_DROPPED_EVENTS,
    VQMBT_EVENT_BLUETOOTH_ENABLED,
    VQMBT_EVENT_BLUETOOTH_DISABLED,
    VQMBT_EVENT_DEVICE_ADDED_REMOVED_CONNECTING,
    VQMBT_EVENT_DEVICE_DISCONNECTED,
    VQMBT_EVENT_DEVICE_CONNECT_SUCCESS,
    VQMBT_EVENT_DEVICE_CONNECT_FAILED,
    VQMBT_EVENT_DEVICE_CONNECT_ABORTED,
    VQMBT_EVENT_DEVICE_CONNECT_CANCELLED,
} VqmbtEventId;

typedef struct VqmbtEvent {
    VqmbtEventId id;
    unsigned int mac0;
    unsigned int mac1;
} VqmbtEvent;

// bluetooth.c syscalls.
VqmbtInferredDevState kvqmbt_device_state(unsigned int mac0, unsigned int mac1);
int kvqmbt_connect_device(unsigned int mac0, unsigned int mac1);
int kvqmbt_disconnect_device(unsigned int mac0, unsigned int mac1);
int kvqmbt_get_paired_devices(VqmbtDeviceInfo* info, int info_size);
bool kvqmbt_bluetooth_state(void);

// umod_callback.c syscalls.
int kvqmbt_read_event(VqmbtEvent* event, int num_events);
SceUID kvqmbt_get_wrapped_event_flag(void);
void kvqmbt_unwrap_event_flag(void);

#endif  // VQMBT_H
