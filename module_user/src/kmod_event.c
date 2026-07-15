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
 * @brief Listen for and handle events emitted by the kernel module.
 ******************************************************************************/

#include "kmod_event.h"

#include <psp2/kernel/error.h>
#include <psp2/kernel/threadmgr.h>
#include <stdbool.h>

#include "log.h"
#include "vqmbt.h"

#define THREAD_PRIORITY 0x96 /* Higher value = lower priority. */
#define THREAD_STACK_SIZE 0x4000

static KmeCallback cb_on_start = NULL;
static KmeCallback cb_on_event_dropped = NULL;
static KmeCallbackEvent cb_on_event = NULL;
static SceUID uid_event_flag = -1;
static SceUID uid_thread = -1;
static _Atomic bool run_thread = false;

/**
 * Retrieve events from the kernel module's ring buffer via syscall and pass them to handle_event().
 */
static void fetch_events(void) {
    while (true) {
        VqmbtEvent event = {0};

        // Fetch event data.
        int ret = kvqmbt_read_event(&event, 1);

        // Handle errors.
        if (ret == VQMBT_ERROR_CB_OVERFLOW) {
            LOG_WARN("kvqmbt_read_event reported dropped events");
            if (cb_on_event_dropped == NULL) {
                LOG_ERROR("cb_on_event_dropped is null");
            } else {
                cb_on_event_dropped();
            }
            continue;
        }
        if (ret < 0) {
            LOG_ERROR("kvqmbt_read_event returned error 0x%08X", ret);
            break;
        }

        // Handle no more events to read.
        if (ret == 0) {
            break;
        }

        // Continue in handler.
        if (cb_on_event == NULL) {
            LOG_ERROR("cb_on_event is null");
        } else {
            cb_on_event(&event);
        }
    }
}

/**
 * Event thread that retrieves the event flag UID from a syscall and waits for bluetooth events from the kernel module.
 *
 * The thread waits for and runs fetch_events() until run_thread signals it to stop.
 *
 * @return Success always.
 */
static int event_thread(SceSize args, void* argp) {
    (void)args;
    (void)argp;

    LOG_DEBUG(0, "Thread started");

    // Run the on_start function if passed.
    if (cb_on_start == NULL) {
        LOG_ERROR("cb_on_start is null");
    } else {
        cb_on_start();
    }

    // Get event flag.
    uid_event_flag = kvqmbt_get_wrapped_event_flag();
    LOG_DEBUG(0, "kvqmbt_get_wrapped_event_flag returned 0x%08X", uid_event_flag);
    if (uid_event_flag < 0) {
        // TODO handle when kmod not loaded https://github.com/Robpol86/vitaQmBluetooth/issues/108
        LOG_ERROR("kvqmbt_get_wrapped_event_flag returned error 0x%08X", uid_event_flag);
        return 0;
    }

    // Run until thread is stopped.
    while (run_thread) {
        // Wait for kernel module event.
        unsigned int timeout = 200 * 1000;
        int ret = sceKernelWaitEventFlag(uid_event_flag, 1, SCE_EVENT_WAITOR | SCE_EVENT_WAITCLEAR_PAT, NULL, &timeout);
        if (ret < 0 && ret != SCE_KERNEL_ERROR_WAIT_TIMEOUT) {
            LOG_ERROR("sceKernelWaitEventFlag returned error 0x%08X", ret);
            break;
        }

        // Fetch and handle events.
        fetch_events();
    }

    // Thread is stopping, clean up.
    kvqmbt_unwrap_event_flag();
    uid_event_flag = -1;

    LOG_DEBUG(0, "Thread exiting");

    return 0;
}

/**
 * Create a thread to handle events from the kernel module and start it.
 *
 * TODO:
 * - Return errors so caller can return non-success.
 *
 * @param on_start Call this function from the thread when it starts.
 * @param on_event_dropped Call this function from the thread when dropped/missed kernel events are detected.
 * @param on_event Call this function from the thread when an event is detected.
 */
void kmod_event_start(KmeCallback on_start, KmeCallback on_event_dropped, KmeCallbackEvent on_event) {
    if (uid_thread >= 0) {
        return;
    }

    cb_on_start = on_start;
    cb_on_event_dropped = on_event_dropped;
    cb_on_event = on_event;
    run_thread = true;

    // Create the thread.
    uid_thread = sceKernelCreateThread("vqmbt-kmod_event-event_thread", event_thread, THREAD_PRIORITY, THREAD_STACK_SIZE,
                                       0, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL);
    if (uid_thread < 0) {
        LOG_ERROR("sceKernelCreateThread returned error 0x%08X", uid_thread);
        return;
    }
    LOG_DEBUG(0, "sceKernelCreateThread returned 0x%08X", uid_thread);

    // Start the thread.
    int ret = sceKernelStartThread(uid_thread, 0, NULL);
    LOG_DEBUG(0, "sceKernelStartThread returned 0x%08X", ret);
}

/**
 * Shut down the running event-handling thread.
 *
 * TODO:
 * - Return errors so caller can return non-success.
 */
void kmod_event_stop(void) {
    if (uid_thread < 0) {
        return;
    }

    // Tell the thread to stop.
    run_thread = false;

    // Wait for thread to stop.
    int ret = sceKernelWaitThreadEnd(uid_thread, NULL, NULL);
    LOG_DEBUG(0, "sceKernelWaitThreadEnd returned 0x%08X", ret);

    // Delete the thread.
    ret = sceKernelDeleteThread(uid_thread);
    LOG_DEBUG(0, "sceKernelDeleteThread returned 0x%08X", ret);

    uid_thread = -1;
}
