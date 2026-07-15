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
 * @brief Syscalls to allow the user plugin to control the bluetooth subsystem.
 ******************************************************************************/

#include <psp2kern/bt.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysclib.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <sce_const.h>
#include <stdbool.h>

#include "log.h"
#include "vqmbt.h"

static SceBtRegisteredInfo paired_devices[VQMBT_MAX_DEVICES];  // TODO locking/semaphore?

/**
 * Return the connection state of the device.
 *
 * @param mac0 First four bytes of the bluetooth device's MAC address.
 * @param mac1 Last two bytes of the bluetooth device's MAC address.
 * @return state ID.
 */
VqmbtInferredDevState kvqmbt_device_state(unsigned int mac0, unsigned int mac1) {
    VqmbtInferredDevState state = ksceBtGetConnectingInfo(mac0, mac1);
    LOG_DEBUG(0, "ksceBtGetConnectingInfo(mac0=%08X, mac1=%08X) returned state=%d", mac0, mac1, state);

    return state;
}

/**
 * Tell the kernel to start connecting to the bluetooth device.
 *
 * @param mac0 First four bytes of the bluetooth device's MAC address.
 * @param mac1 Last two bytes of the bluetooth device's MAC address.
 * @return 0 on success, negative on failure.
 */
int kvqmbt_connect_device(unsigned int mac0, unsigned int mac1) {
    int ret = ksceBtStartConnect(mac0, mac1);
    if (ret >= 0) {
        LOG_DEBUG(0, "ksceBtStartConnect(mac0=%08X, mac1=%08X) returned %d", mac0, mac1, ret);
        return 0;
    }

    switch (ret) {
        case SCE_BT_ERROR_CONNECT_START_NOT_CONNECTABLE:
            LOG_ERROR("ksceBtStartConnect(mac0=%08X, mac1=%08X) returned SCE_BT_ERROR_CONNECT_START_NOT_CONNECTABLE",
                      mac0, mac1);
            return VQMBT_ERROR_KERNEL_SIDE_NOT_CONNECTABLE;
        case SCE_BT_ERROR_CONNECT_START_BUSY:
            LOG_ERROR("ksceBtStartConnect(mac0=%08X, mac1=%08X) returned SCE_BT_ERROR_CONNECT_START_BUSY", mac0, mac1);
            return VQMBT_ERROR_KERNEL_SIDE_BUSY;
        default:
            LOG_ERROR("ksceBtStartConnect(mac0=%08X, mac1=%08X) returned error 0x%08X", mac0, mac1, ret);
            return ret;
    }
}

/**
 * Tell the kernel to start disconnecting the bluetooth device.
 *
 * @param mac0 First four bytes of the bluetooth device's MAC address.
 * @param mac1 Last two bytes of the bluetooth device's MAC address.
 * @return 0 on success, negative on failure.
 */
int kvqmbt_disconnect_device(unsigned int mac0, unsigned int mac1) {
    int ret = ksceBtStartDisconnect(mac0, mac1);
    if (ret >= 0) {
        LOG_DEBUG(0, "ksceBtStartDisconnect(mac0=%08X, mac1=%08X) returned %d", mac0, mac1, ret);
        return 0;
    }

    switch (ret) {
        default:
            LOG_ERROR("ksceBtStartDisconnect(mac0=%08X, mac1=%08X) returned error 0x%08X", mac0, mac1, ret);
            return ret;
    }
}

/**
 * Get all currently paired bluetooth devices.
 *
 * @param info User-space pointer to an array of VqmbtDeviceInfo records.
 * @param info_size Capacity of the user array, in records.
 * @return Number of records written on success, or a negative error code on failure.
 */
int kvqmbt_get_paired_devices(VqmbtDeviceInfo* info, int info_size) {
    // Validate.
    if (info == NULL || info_size < 1 || info_size > VQMBT_MAX_DEVICES) {
        LOG_ERROR("Invalid argument: info=%p info_size=%d", info, info_size);
        return VQMBT_ERROR_INVALID_ARGUMENT;
    }

    // Zero the kernel-side array to prevent ghost data.
    memset(paired_devices, 0, sizeof(paired_devices));

    // Populate file-scoped array with all currently paired devices.
    int count = ksceBtGetRegisteredInfo(0, 0, paired_devices, info_size);
    if (count < 0) {
        LOG_ERROR("ksceBtGetRegisteredInfo returned error 0x%08X", count);
        return VQMBT_ERROR_KERNEL_SIDE;
    }
    LOG_DEBUG(0, "ksceBtGetRegisteredInfo returned count=%d info_size=%d max=%d", count, info_size, VQMBT_MAX_DEVICES);

    // Copy each record across the kernel/user boundary.
    VqmbtInferredDevState state = 0;
    for (int idx = 0; idx < count; idx++) {
        // Initialize user side.
        VqmbtDeviceInfo dev = {0};

        // Get kernel side.
        SceBtRegisteredInfo* sceDev = &paired_devices[idx];
        const unsigned char* mac = (const unsigned char*)&sceDev->mac;
        LOG_DEBUG(0, "idx=%d name=\"%s\" mac=%02X:%02X:%02X:%02X:%02X:%02X class=0x%08X vid=0x%04X pid=0x%04X", idx,
                  sceDev->name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], sceDev->bt_class, sceDev->vid,
                  sceDev->pid);
        LOG_DEBUG(0, "      unk0=0x%04X unk1=0x%08X unk2=0x%08X unk3=0x%08X unk4=0x%08X", sceDev->unk0, sceDev->unk1,
                  sceDev->unk2, sceDev->unk3, sceDev->unk4);
        for (int row = 0; row < 0x60; row += 16) {
            LOG_DEBUG(
                10000,
                "      unk5[0x%02X]=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", row,
                sceDev->unk5[row + 0], sceDev->unk5[row + 1], sceDev->unk5[row + 2], sceDev->unk5[row + 3],
                sceDev->unk5[row + 4], sceDev->unk5[row + 5], sceDev->unk5[row + 6], sceDev->unk5[row + 7],
                sceDev->unk5[row + 8], sceDev->unk5[row + 9], sceDev->unk5[row + 10], sceDev->unk5[row + 11],
                sceDev->unk5[row + 12], sceDev->unk5[row + 13], sceDev->unk5[row + 14], sceDev->unk5[row + 15]);
        }

        // Set name and uint macs.
        strncpy(dev.name, sceDev->name, sizeof(dev.name) - 1);
        dev.name[sizeof(dev.name) - 1] = '\0';
        dev.mac0 = ((unsigned int)mac[3] << 24) | ((unsigned int)mac[2] << 16) | ((unsigned int)mac[1] << 8) | mac[0];
        dev.mac1 = ((unsigned int)mac[5] << 8) | mac[4];

        // Get/set state.
        state = ksceBtGetConnectingInfo(dev.mac0, dev.mac1);
        LOG_DEBUG(0, "ksceBtGetConnectingInfo(mac0=%08X, mac1=%08X) returned state=%d", dev.mac0, dev.mac1, state);
        dev.state = state;

        // Export to user space.
        int ret = ksceKernelCopyToUser(&info[idx], &dev, sizeof(dev));
        if (ret < 0) {
            LOG_ERROR("ksceKernelCopyToUser failed at index %d: 0x%08X", idx, ret);
            return VQMBT_ERROR_KERNEL_SIDE;
        }
    }

    return count;
}

/**
 * Get the bluetooth subsystem state.
 *
 * @return True if bluetooth is enabled.
 */
bool kvqmbt_bluetooth_state(void) {
    int state = ksceBtGetConfiguration();
    LOG_DEBUG(0, "ksceBtGetConfiguration returned state=0x%08X", state);

    return state == 0x9;
}
