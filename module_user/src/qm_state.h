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
 * @brief Quick Menu state management interface for the rest of the module.
 ******************************************************************************/

#ifndef QM_STATE_H
#define QM_STATE_H

#include <stdbool.h>

#include "vqmbt.h"

// UI request.
typedef enum QmsRequestId : unsigned int {
    QMS_REQUEST_BULK_UPDATE,
    QMS_REQUEST_BUTTON_PRESSED,
    QMS_REQUEST_BLUETOOTH_ON,
    QMS_REQUEST_BLUETOOTH_OFF,
    QMS_REQUEST_DEVICE_DISCONNECTED,
    QMS_REQUEST_DEVICE_CONNECTED,
} QmsRequestId;
typedef struct QmsRequest {
    QmsRequestId id;
    union {
        int idx;
        struct {
            unsigned int mac0, mac1;
        } mac;
        struct {
            bool bluetooth_on;
            int num_devices;
            const VqmbtDeviceInfo* devices;
        } bulk;
    };
} QmsRequest;

// Exported functions.
bool qm_state_update_ui(const QmsRequest* request);
int qm_state_mutex_start(void);
int qm_state_mutex_stop(void);

#endif  // QM_STATE_H
