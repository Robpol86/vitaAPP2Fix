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
 * @brief Send events to the user module.
 ******************************************************************************/

#include "umod_callback.h"

#include <psp2kern/kernel/sysmem/data_transfers.h>
#include <psp2kern/kernel/sysmem/uid_puid.h>
#include <psp2kern/kernel/threadmgr.h>
#include <stdatomic.h>

#include "log.h"
#include "vqmbt.h"

#define RING_BUFFER_SIZE 16

static VqmbtEvent ring_buffer[RING_BUFFER_SIZE];
static _Atomic unsigned int ring_buffer_write_idx = 0;
static _Atomic unsigned int ring_buffer_read_idx = 0;
static SceUID event_flag_uid = -1;
static SceUID event_flag_puid = -1;
static SceUID event_flag_user_pid = -1;

/**
 * Write a new event into the ring buffer and notify the user module of the new event.
 *
 * @param event Event details.
 * @return 0 on success, negative on error.
 */
int umod_cb_emit_event(const VqmbtEvent* event) {
    if (event == NULL || event->id == VQMBT_EVENT_NONE) {
        return VQMBT_ERROR_INVALID_ARGUMENT;
    }

    // Write event into ring buffer atomically.
    unsigned int write_idx = atomic_load_explicit(&ring_buffer_write_idx, memory_order_relaxed);
    int slot_no = write_idx % RING_BUFFER_SIZE;
    ring_buffer[slot_no] = *event;
    atomic_store_explicit(&ring_buffer_write_idx, write_idx + 1, memory_order_release);
    LOG_DEBUG(0, "Wrote event to slot_no=%d (write_idx=0x%08X)", slot_no, write_idx);

    // Tell the consumer an event is ready to be read.
    if (event_flag_uid >= 0) {
        int ret = ksceKernelSetEventFlag(event_flag_uid, 1);
        if (ret < 0) {
            LOG_ERROR("ksceKernelSetEventFlag(event_flag_uid=0x%08X) returned error 0x%08X", event_flag_uid, ret);
        } else {
            LOG_DEBUG(0, "ksceKernelSetEventFlag(event_flag_uid=0x%08X) returned %d", event_flag_uid, ret);
        }
    }

    return 0;
}

/**
 * Syscall that the user module uses to read the next event from the ring buffer.
 *
 * @param event Write event details into this pointer.
 * @param num_events Just reset read_idx if <1, return 1 if 1, error on >1.
 * @return 0 on no new event, 1 on new event, negative on error.
 */
int kvqmbt_read_event(VqmbtEvent* event, int num_events) {
    if (num_events > 1) {
        LOG_ERROR("num_events must be <=1: %d", num_events);
        return VQMBT_ERROR_INVALID_ARGUMENT;
    }
    if (num_events > 0 && event == NULL) {
        LOG_ERROR("Invalid event pointer: NULL");
        return VQMBT_ERROR_INVALID_ARGUMENT;
    }

    // Atomically read indices.
    unsigned int read_idx = atomic_load_explicit(&ring_buffer_read_idx, memory_order_relaxed);
    unsigned int write_idx = atomic_load_explicit(&ring_buffer_write_idx, memory_order_acquire);
    if (read_idx == write_idx) {
        // No new data.
        return 0;
    }
    LOG_DEBUG(0, "Fetched write_idx=0x%08X read_idx=0x%08X", write_idx, read_idx);

    // Reset read_idx if requested.
    if (num_events < 1) {
        atomic_store_explicit(&ring_buffer_read_idx, write_idx, memory_order_relaxed);
        LOG_DEBUG(0, "Set ring_buffer_read_idx=0x%08X", write_idx);
        return 0;
    }

    // Fault if producer wrote more since the last read than the size of the buffer.
    if (write_idx - read_idx > RING_BUFFER_SIZE) {
        LOG_WARN("Ring buffer overrun: write_idx=%u read_idx=%u (delta=%u)", write_idx, read_idx, write_idx - read_idx);
        atomic_store_explicit(&ring_buffer_read_idx, write_idx, memory_order_relaxed);
        return VQMBT_ERROR_CB_OVERFLOW;
    }

    // Read event from buffer, then check again if buffer was overrun during the read.
    VqmbtEvent event_from_buffer = ring_buffer[read_idx % RING_BUFFER_SIZE];
    write_idx = atomic_load_explicit(&ring_buffer_write_idx, memory_order_acquire);
    if (write_idx - read_idx > RING_BUFFER_SIZE) {
        LOG_WARN("Ring buffer overrun mid-read: write_idx=%u read_idx=%u (delta=%u)", write_idx, read_idx,
                 write_idx - read_idx);
        atomic_store_explicit(&ring_buffer_read_idx, write_idx, memory_order_relaxed);
        return VQMBT_ERROR_CB_OVERFLOW;
    }
    atomic_store_explicit(&ring_buffer_read_idx, read_idx + 1, memory_order_relaxed);  // Commit.

    // Export to user space.
    int ret = ksceKernelCopyToUser(event, &event_from_buffer, sizeof(event_from_buffer));
    if (ret < 0) {
        LOG_ERROR("ksceKernelCopyToUser returned error 0x%08X", ret);
        return VQMBT_ERROR_KERNEL_SIDE;
    }

    return 1;
}

/**
 * Wrap the kernel-context event flag UID into a UID accessible from the user module's context.
 *
 * @return The wrapped event flag UID on success, negative on error.
 */
SceUID kvqmbt_get_wrapped_event_flag(void) {
    // Sanity check.
    if (event_flag_uid < 0) {
        LOG_ERROR("event_flag_uid is not initialized: 0x%08X", event_flag_uid);
        return VQMBT_ERROR_NOT_READY;
    }
    if (event_flag_puid >= 0) {
        // Already wrapped.
        LOG_DEBUG(0, "Already wrapped");
        return event_flag_puid;
    }

    // Get the caller's pid (e.g. the user module's pid when calling this via syscall).
    event_flag_user_pid = ksceKernelGetProcessId();
    if (event_flag_user_pid < 0) {
        LOG_ERROR("ksceKernelGetProcessId returned error 0x%08X", event_flag_user_pid);
        return event_flag_user_pid;
    }
    LOG_DEBUG(0, "ksceKernelGetProcessId returned pid=0x%08X", event_flag_user_pid);

    // Wrap event_flag_uid into a uid the user module can access.
    event_flag_puid = ksceKernelCreateUserUid(event_flag_user_pid, event_flag_uid);
    if (event_flag_puid < 0) {
        LOG_ERROR("ksceKernelCreateUserUid returned error 0x%08X", event_flag_puid);
        return event_flag_puid;
    }
    LOG_DEBUG(0, "ksceKernelCreateUserUid returned event_flag_puid=0x%08X", event_flag_puid);

    return event_flag_puid;
}

/**
 * Delete the wrapped event flag's "wrapping".
 */
void kvqmbt_unwrap_event_flag(void) {
    if (event_flag_puid < 0) {
        return;
    }

    int ret = ksceKernelDeleteUserUid(event_flag_user_pid, event_flag_puid);
    if (ret < 0) {
        LOG_ERROR("ksceKernelDeleteUserUid(pid=0x%08X, puid=0x%08X) returned error 0x%08X", event_flag_user_pid,
                  event_flag_puid, ret);
        return;
    }
    LOG_DEBUG(0, "ksceKernelDeleteUserUid(pid=0x%08X, puid=0x%08X) returned 0x%08X", event_flag_user_pid, event_flag_puid,
              ret);

    event_flag_puid = -1;
    event_flag_user_pid = -1;
}

/**
 * Initialize the event flag used to send signals to the user module.
 *
 * @return 0 on success, negative on error.
 */
int umod_cb_start(void) {
    // Create an event flag.
    event_flag_uid = ksceKernelCreateEventFlag("kvqmbt-umod_callback-event_flag", 0, 0, NULL);
    if (event_flag_uid < 0) {
        LOG_ERROR("ksceKernelCreateEventFlag returned error 0x%08X", event_flag_uid);
        return VQMBT_ERROR_KERNEL_SIDE;
    }
    LOG_DEBUG(0, "ksceKernelCreateEventFlag returned event_flag_uid=0x%08X", event_flag_uid);

    return 0;
}

/**
 * Cleanup and de-initialize the event flag.
 *
 * @return 0 on success, negative on error.
 */
int umod_cb_stop(void) {
    // Delete wrapped event flag uid.
    kvqmbt_unwrap_event_flag();

    // Delete the event flag.
    if (event_flag_uid >= 0) {
        int ret = ksceKernelDeleteEventFlag(event_flag_uid);
        if (ret < 0) {
            LOG_ERROR("ksceKernelDeleteEventFlag returned error 0x%08X", ret);
            return VQMBT_ERROR_KERNEL_SIDE;
        }
        LOG_DEBUG(0, "ksceKernelDeleteEventFlag returned 0x%08X", ret);
        event_flag_uid = -1;
    }

    return 0;
}
