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
 * @brief Manage the state of buttons in the Quick Menu.
 ******************************************************************************/

#include "qm_state.h"

#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/threadmgr/lw_mutex.h>
#include <psp2/kernel/threadmgr/thread.h>
#include <quickmenureborn/c_types.h>
#include <quickmenureborn/qm_reborn.h>

#include "log.h"
#include "qm_ids.h"
#include "sce_const.h"
#include "vqmbt.h"

#define BUTTON_LABEL_MAX (VQMBT_SCE_DEVICE_NAME_MAX + 16)

static SceKernelLwMutexWork mutex;
static bool first_run = true;

// Button finite states.
typedef enum QmButtonState : unsigned int {
    BTNSTATE_SLOT_EMPTY_DISABLED,
    BTNSTATE_BT_OFF_DISABLED,
    BTNSTATE_DISCONNECTED,
    BTNSTATE_DISCONNECTING_DISABLED,
    BTNSTATE_CONNECTED,
    BTNSTATE_CONNECTING_DISABLED,
    BTNSTATE_ERROR_DISABLED,
} QmButtonState;

// Button descriptor.
typedef struct QmButton {
    VqmbtDeviceInfo device;
    QmButtonState btn_state;
    char error_message[BUTTON_LABEL_MAX];
} QmButton;

// Quickmenu descriptor.
typedef struct QmState {
    QmButton buttons[VQMBT_MAX_DEVICES];
} QmState;

// Quickmenu state.
static QmState qm_state;

/**
 * Read the current state and update the UI to make it match.
 */
static void refresh_ui(void) {
    for (int idx = 0; idx < VQMBT_MAX_DEVICES; idx++) {
        const QmButton* qm_button = &qm_state.buttons[idx];
        const VqmbtDeviceInfo* device = &qm_button->device;
        char label[BUTTON_LABEL_MAX];
        bool button_enabled = false;

        // Determine label.
        switch (qm_button->btn_state) {
            case BTNSTATE_SLOT_EMPTY_DISABLED:
                sceClibSnprintf(label, sizeof(label), "Slot %d: no device", idx + 1);
                break;
            case BTNSTATE_BT_OFF_DISABLED:
                sceClibSnprintf(label, sizeof(label), "Bluetooth is disabled");
                break;
            case BTNSTATE_DISCONNECTING_DISABLED:
                sceClibSnprintf(label, sizeof(label), "Disconnecting %s", device->name);
                break;
            case BTNSTATE_CONNECTING_DISABLED:
                sceClibSnprintf(label, sizeof(label), "Connecting %s", device->name);
                break;
            case BTNSTATE_ERROR_DISABLED:
                sceClibSnprintf(label, sizeof(label), "Error: %s", qm_button->error_message);
                break;
            case BTNSTATE_DISCONNECTED:
                sceClibSnprintf(label, sizeof(label), "Connect %s", device->name[0] ? device->name : "N/A");
                button_enabled = true;
                break;
            case BTNSTATE_CONNECTED:
                sceClibSnprintf(label, sizeof(label), "Disconnect %s", device->name);
                button_enabled = true;
                break;
        }

        // Update label.
        const char* id = QM_ID_BUTTONS[idx];
        QuickMenuRebornSetWidgetLabel(id, label);

        // Enable/disable button.
        if (button_enabled) {
            // Enable button.
            QuickMenuRebornSetWidgetColor(id, 1.0F, 1.0F, 1.0F, 1.0F);
        } else {
            // Disable button.
            QuickMenuRebornSetWidgetColor(id, 0.5F, 0.5F, 0.5F, 1.0F);
        }
    }
}

/**
 * Transition one or all buttons to BTNSTATE_BT_OFF_DISABLED.
 *
 * @param changed Set to true if one or more states were changed.
 * @param idx Read/change state for qm_state.buttons[idx] or all buttons if NULL.
 */
static void transition_state_bt_off(bool* changed, const int* idx) {
    if (idx == NULL) {
        LOG_DEBUG(0, "Setting all occupied slots as bluetooth off");
        for (int i = 0; i < VQMBT_MAX_DEVICES; i++) {
            transition_state_bt_off(changed, &i);
        }
        return;
    }
    QmButton* qm_button = &qm_state.buttons[*idx];
    if (qm_button->btn_state != BTNSTATE_BT_OFF_DISABLED && qm_button->btn_state != BTNSTATE_SLOT_EMPTY_DISABLED) {
        LOG_DEBUG(0, "Setting idx=%d as bluetooth off", *idx);
        qm_button->btn_state = BTNSTATE_BT_OFF_DISABLED;
        *changed = true;
        return;
    }
    LOG_DEBUG(0, "No-op");
}

/**
 * Transition one button to BTNSTATE_DISCONNECTED.
 *
 * Does not move the button off the BTNSTATE_BT_OFF_DISABLED state if `force` is false. The reason for this guard is
 * because if a bluetooth device is connected and the user turns off the bluetooth feature, the bluetooth-off event is
 * emitted before the device-disconnect event.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Read/change state for qm_state.buttons[idx].
 * @param force If false no-op if button is in the BTNSTATE_BT_OFF_DISABLED state.
 */
static void transition_state_disconnected(bool* changed, const int idx, bool force) {
    QmButton* qm_button = &qm_state.buttons[idx];
    if (qm_button->btn_state == BTNSTATE_BT_OFF_DISABLED && !force) {
        LOG_DEBUG(0, "No-op");
        return;
    }
    if (qm_button->btn_state != BTNSTATE_DISCONNECTED) {
        LOG_DEBUG(0, "Setting idx=%d as disconnected", idx);
        qm_button->btn_state = BTNSTATE_DISCONNECTED;
        *changed = true;
        return;
    }
    LOG_DEBUG(0, "No-op");
}

/**
 * Transition one button to BTNSTATE_DISCONNECTING_DISABLED.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Read/change state for qm_state.buttons[idx].
 */
static void transition_state_busy_disconnecting(bool* changed, const int idx) {
    QmButton* qm_button = &qm_state.buttons[idx];
    if (qm_button->btn_state != BTNSTATE_DISCONNECTING_DISABLED) {
        LOG_DEBUG(0, "Setting idx=%d as disconnecting", idx);
        qm_button->btn_state = BTNSTATE_DISCONNECTING_DISABLED;
        *changed = true;
        return;
    }
    LOG_DEBUG(0, "No-op");
}

/**
 * Transition one button to BTNSTATE_CONNECTED.
 *
 * Does not move the button off the BTNSTATE_BT_OFF_DISABLED state if `force` is false. The reason for this guard is
 * because if the user turns off bluetooth via the Quick Menu, then goes into settings and turns it back on and connects a
 * device, then opens the Quick Menu again, we want the bulk update to show the true state of the device.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Read/change state for qm_state.buttons[idx].
 * @param force If false no-op if button is in the BTNSTATE_BT_OFF_DISABLED state.
 */
static void transition_state_connected(bool* changed, const int idx, bool force) {
    QmButton* qm_button = &qm_state.buttons[idx];
    if (qm_button->btn_state == BTNSTATE_BT_OFF_DISABLED && !force) {
        LOG_DEBUG(0, "No-op");
        return;
    }
    if (qm_button->btn_state != BTNSTATE_CONNECTED) {
        LOG_DEBUG(0, "Setting idx=%d as connected", idx);
        qm_button->btn_state = BTNSTATE_CONNECTED;
        *changed = true;
        return;
    }
    LOG_DEBUG(0, "No-op");
}

/**
 * Transition one button to BTNSTATE_CONNECTING_DISABLED.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Read/change state for qm_state.buttons[idx].
 */
static void transition_state_busy_connecting(bool* changed, const int idx) {
    QmButton* qm_button = &qm_state.buttons[idx];
    if (qm_button->btn_state != BTNSTATE_CONNECTING_DISABLED) {
        LOG_DEBUG(0, "Setting idx=%d as connecting", idx);
        qm_button->btn_state = BTNSTATE_CONNECTING_DISABLED;
        *changed = true;
        return;
    }
    LOG_DEBUG(0, "No-op");
}

/**
 * Transition one button to BTNSTATE_ERROR_DISABLED and set an error message.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Read/change state for qm_state.buttons[idx].
 * @param error The error code.
 */
static void transition_state_error(bool* changed, const int idx, const int error) {
    QmButton* qm_button = &qm_state.buttons[idx];

    if (qm_button->btn_state == BTNSTATE_ERROR_DISABLED) {
        LOG_DEBUG(0, "No-op");
        return;
    }

    LOG_DEBUG(0, "Setting idx=%d as error=0x%08X", idx, error);
    qm_button->btn_state = BTNSTATE_ERROR_DISABLED;

    switch (error) {
        case VQMBT_ERROR_KERNEL_SIDE_NOT_CONNECTABLE:
            sceClibSnprintf(qm_button->error_message, sizeof(qm_button->error_message), "%s", "Must connect from device");
            break;
        case VQMBT_ERROR_KERNEL_SIDE_BUSY:
            sceClibSnprintf(qm_button->error_message, sizeof(qm_button->error_message), "%s", "Busy (Settings opened?)");
            break;
        default:
            sceClibSnprintf(qm_button->error_message, sizeof(qm_button->error_message), "0x%08X", error);
            break;
    }

    *changed = true;
}

/**
 * Connect or disconnect a bluetooth device. Called when user presses on one of the buttons.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Which button was pressed.
 */
static void button_pressed(bool* changed, const int idx) {
    QmButton* qm_button = &qm_state.buttons[idx];
    const VqmbtDeviceInfo* device = &qm_button->device;

    switch (qm_button->btn_state) {
        case BTNSTATE_SLOT_EMPTY_DISABLED:
        case BTNSTATE_BT_OFF_DISABLED:
        case BTNSTATE_DISCONNECTING_DISABLED:
        case BTNSTATE_CONNECTING_DISABLED:
        case BTNSTATE_ERROR_DISABLED:
            LOG_DEBUG(0, "Button idx=%d pressed but disabled, ignoring", idx);
            return;
        case BTNSTATE_DISCONNECTED:
            LOG_DEBUG(0, "Connecting device \"%s\"", device->name);
            int ret = kvqmbt_connect_device(device->mac0, device->mac1);
            if (ret < 0) {
                transition_state_error(changed, idx, ret);
            } else {
                transition_state_busy_connecting(changed, idx);
            }
            break;
        case BTNSTATE_CONNECTED:
            LOG_DEBUG(0, "Disconnecting device \"%s\"", device->name);
            ret = kvqmbt_disconnect_device(device->mac0, device->mac1);
            if (ret < 0) {
                transition_state_error(changed, idx, ret);
            } else {
                transition_state_busy_disconnecting(changed, idx);
            }
            break;
    }
}

/**
 * Transition all non-empty buttons out of BTNSTATE_BT_OFF_DISABLED.
 *
 * @param changed Set to true if the state was changed.
 */
static void transition_state_bt_on(bool* changed) {
    for (int idx = 0; idx < VQMBT_MAX_DEVICES; idx++) {
        const QmButton* qm_button = &qm_state.buttons[idx];
        const VqmbtDeviceInfo* device = &qm_button->device;

        // Skip if empty.
        if (qm_button->btn_state == BTNSTATE_SLOT_EMPTY_DISABLED) {
            LOG_DEBUG(0, "Skipping empty slot idx=%d", idx);
            continue;
        }

        LOG_DEBUG(0, "Restoring state for slot idx=%d", idx);

        // Restore state.  // TODO DRY.
        switch (device->state) {
            case VQMBT_SCE_BT_STATE_DISCONNECTED:
                transition_state_disconnected(changed, idx, true);
                break;
            case VQMBT_SCE_BT_STATE_CONNECTING:
                transition_state_busy_connecting(changed, idx);
                break;
            case VQMBT_SCE_BT_STATE_DISCONNECTING:
                transition_state_busy_disconnecting(changed, idx);
                break;
            case VQMBT_SCE_BT_STATE_CONNECTED_HID:
            case VQMBT_SCE_BT_STATE_CONNECTED:
            case VQMBT_SCE_BT_STATE_REGISTERING:
                transition_state_connected(changed, idx, false);
                break;
            default:
                transition_state_disconnected(changed, idx, false);
                break;
        }
    }
}

/**
 * Transition one button to BTNSTATE_SLOT_EMPTY_DISABLED.
 *
 * @param changed Set to true if the state was changed.
 * @param idx Read/change state for qm_state.buttons[idx].
 */
static void bulk_updated_remove_device(bool* changed, const int idx) {
    QmButton* qm_button = &qm_state.buttons[idx];
    if (qm_button->btn_state != BTNSTATE_SLOT_EMPTY_DISABLED) {
        LOG_DEBUG(0, "Setting idx=%d as empty", idx);
        sceClibMemset(&qm_button->device, 0, sizeof(qm_button->device));
        qm_button->btn_state = BTNSTATE_SLOT_EMPTY_DISABLED;
        *changed = true;
        return;
    }
    LOG_DEBUG(0, "No-op");
}

/**
 * Bulk update all buttons regardless of prior states.
 *
 * @param changed Set to true if the state was changed.
 * @param request State change request payload.
 */
static void bulk_update(bool* changed, const QmsRequest* request) {
    for (int idx = 0; idx < VQMBT_MAX_DEVICES; idx++) {
        LOG_DEBUG(0, "Bulk updating idx=%d", idx);

        // Check if device was removed.
        if (idx >= request->bulk.num_devices) {
            LOG_DEBUG(0, "No device");
            bulk_updated_remove_device(changed, idx);
            continue;
        }

        const VqmbtDeviceInfo* new_device = &request->bulk.devices[idx];
        QmButton* qm_button = &qm_state.buttons[idx];
        VqmbtDeviceInfo* old_device = &qm_button->device;

        // Update button device data if it has a new device.
        if (new_device->mac0 != old_device->mac0 || new_device->mac1 != old_device->mac1) {
            LOG_DEBUG(0, "New device");
            sceClibMemcpy(old_device, new_device, sizeof(*new_device));
            qm_button->btn_state = BTNSTATE_DISCONNECTED;
            *changed = true;
        } else {
            LOG_DEBUG(0, "Old device");
        }

        // Check if bluetooth is off.
        if (!request->bulk.bluetooth_on) {
            LOG_DEBUG(0, "Old device and bluetooth is off");
            transition_state_bt_off(changed, &idx);
            continue;
        }

        // Set state.
        switch (new_device->state) {
            case VQMBT_SCE_BT_STATE_CONNECTING:
                LOG_DEBUG(0, "Setting state to connecting if not set");
                transition_state_busy_connecting(changed, idx);
                break;
            case VQMBT_SCE_BT_STATE_DISCONNECTING:
                LOG_DEBUG(0, "Setting state to disconnecting if not set");
                transition_state_busy_disconnecting(changed, idx);
                break;
            case VQMBT_SCE_BT_STATE_CONNECTED_HID:
            case VQMBT_SCE_BT_STATE_CONNECTED:
            case VQMBT_SCE_BT_STATE_REGISTERING:
                LOG_DEBUG(0, "Setting state to connected if not set");
                transition_state_connected(changed, idx, true);
                break;
            default:
                LOG_DEBUG(0, "Setting state to disconnected if not set");
                transition_state_disconnected(changed, idx, true);
                break;
        }
    }
}

/**
 * Get the index of the button for the device with the given MAC address.
 *
 * @param mac0 First four bytes of the bluetooth device's MAC address.
 * @param mac1 Last two bytes of the bluetooth device's MAC address.
 * @return idx of the matching button, negative on error.
 */
static int mac_to_idx(const unsigned int mac0, const unsigned int mac1) {
    if (mac0 < 1 || mac1 < 1) {
        LOG_ERROR("Invalid MAC address: mac0=0x%08X mac1=0x%08X", mac0, mac1);
        return VQMBT_ERROR_INVALID_ARGUMENT;
    }
    for (int idx = 0; idx < VQMBT_MAX_DEVICES; idx++) {
        const VqmbtDeviceInfo* device = &qm_state.buttons[idx].device;
        if (device->mac0 == mac0 && device->mac1 == mac1) {
            return idx;
        }
    }
    LOG_ERROR("MAC address not found: mac0=0x%08X mac1=0x%08X", mac0, mac1);
    return VQMBT_ERROR_INVALID_ARGUMENT;
}

/**
 * Main entrypoint for all events. Callers call this requesting state changes or actions.
 *
 * Forces serialization of threaded events using a mutex.
 *
 * @param request State change request payload.
 * @return True if anything was changed.
 */
bool qm_state_update_ui(const QmsRequest* request) {
    // Lock mutex.
    LOG_DEBUG(0, "Asking for mutex lock");
    sceKernelLockLwMutex(&mutex, 1, NULL);  // Blocks until any existing ongoing request finishes.
    LOG_DEBUG(0, "Obtained mutex lock");

    bool changed = false;

    switch (request->id) {
        case QMS_REQUEST_BULK_UPDATE: {
            LOG_DEBUG(0, "Request bulk update %d devices", request->bulk.num_devices);
            bulk_update(&changed, request);
            break;
        }

        case QMS_REQUEST_BUTTON_PRESSED: {
            LOG_DEBUG(0, "Request button idx=%d pressed", request->idx);
            button_pressed(&changed, request->idx);
            break;
        }

        case QMS_REQUEST_BLUETOOTH_ON: {
            LOG_DEBUG(0, "Request bluetooth on");
            transition_state_bt_on(&changed);
            break;
        }

        case QMS_REQUEST_BLUETOOTH_OFF: {
            LOG_DEBUG(0, "Request bluetooth off");
            transition_state_bt_off(&changed, NULL);
            break;
        }

        case QMS_REQUEST_DEVICE_DISCONNECTED: {
            LOG_DEBUG(0, "Request device mac0=0x%08X mac1=0x%08X disconnected", request->mac.mac0, request->mac.mac1);
            int idx = mac_to_idx(request->mac.mac0, request->mac.mac1);
            if (idx < 0) {
                LOG_ERROR("mac0=0x%08X mac1=0x%08X not found", request->mac.mac0, request->mac.mac1);
            } else {
                transition_state_disconnected(&changed, idx, false);
            }
            break;
        }

        case QMS_REQUEST_DEVICE_CONNECTED: {
            LOG_DEBUG(0, "Request device mac0=0x%08X mac1=0x%08X connected", request->mac.mac0, request->mac.mac1);
            int idx = mac_to_idx(request->mac.mac0, request->mac.mac1);
            if (idx < 0) {
                LOG_ERROR("mac0=0x%08X mac1=0x%08X not found", request->mac.mac0, request->mac.mac1);
            } else {
                transition_state_connected(&changed, idx, false);
            }
            break;
        }
    }

    // Refresh UI.
    if (changed) {
        LOG_DEBUG(0, "Refreshing UI to show changes");
        refresh_ui();
    } else if (first_run) {
        LOG_DEBUG(0, "Refreshing UI to show changes for the first run");
        refresh_ui();
        first_run = false;
    }

    // Release mutex.
    sceKernelUnlockLwMutex(&mutex, 1);
    LOG_DEBUG(0, "Released mutex lock");

    return changed;
}

/**
 * Initialize the mutex to serialize multi-threaded event handling.
 *
 * @return 0 on success, negative on error.
 */
int qm_state_mutex_start(void) {
    int ret = sceKernelCreateLwMutex(&mutex, "vqmbt-qm_state-mutex", 0, 0, NULL);
    if (ret < 0) {
        LOG_ERROR("sceKernelCreateLwMutex returned error 0x%08X", ret);
        return VQMBT_ERROR_GENERAL_FAILURE;
    }
    LOG_DEBUG(0, "sceKernelCreateLwMutex returned 0x%08X", ret);

    return 0;
}

/**
 * Delete the mutex.
 *
 * @return 0 on success, negative on error.
 */
int qm_state_mutex_stop(void) {
    int ret = sceKernelDeleteLwMutex(&mutex);
    if (ret < 0) {
        LOG_ERROR("sceKernelDeleteLwMutex returned error 0x%08X", ret);
        return VQMBT_ERROR_GENERAL_FAILURE;
    }
    LOG_DEBUG(0, "sceKernelDeleteLwMutex returned 0x%08X", ret);

    return 0;
}
