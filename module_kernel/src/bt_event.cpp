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
 * @brief Listen for and handle events emitted by the bluetooth subsystem.
 ******************************************************************************/

/**
 * TODOs:
 * - Revisit atomic run_thread.
 */

#include "bt_event.h"

#include <psp2kern/bt.h>
#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysclib.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <stdbool.h>

#include "log.h"
#include "sce_const.h"
#include "vapptf.h"

#define THREAD_PRIORITY 0x95 /* Higher value = lower priority. */
#define THREAD_STACK_SIZE 0x1000

#define PREFIX "SceBtEvent: "
#define INDENT "            "
static_assert(sizeof(PREFIX) == sizeof(INDENT), "INDENT width must match PREFIX");

static SceUID uid_callback = -1;
static SceUID uid_thread = -1;
static bool run_thread = false;

/**
 * Handle scenario where one or more events went missing.
 */
static void handle_event_dropped(void) {
    // TODO
    LOG_DEBUG(0, "Event drop detected");
}

/**
 * Handler for one event. Called once per bluetooth event.
 *
 * @param event Event details.
 */
static void handle_event(const SceBtEvent* event) {
    LOG_DEBUG(0, PREFIX "id=0x%02X unk1=0x%02X unk3=0x%08X mac0=0x%08X mac1=0x%08X unk2=0x%04X", event->id, event->unk1,
              event->unk3, event->mac0, event->mac1, event->unk2);

    // Log device name in debug builds.
#ifndef NDEBUG
    if (event->mac0 > 0 && event->mac1 > 0) {
        char name[VAPPTF_SCE_DEVICE_NAME_MAX];
        int ret = ksceBtGetDeviceName(event->mac0, event->mac1, name);
        if (ret == 0) {
            LOG_DEBUG(0, INDENT "Name: \"%s\"", name);
        } else {
            LOG_ERROR("ksceBtGetDeviceName(mac0=0x%08X, mac1=0x%08X) returned error 0x%08X", event->mac0, event->mac1,
                      ret);
        }
    }
#endif  // NDEBUG

    // Log known event name.
#ifndef NDEBUG
    switch (event->id) {
#define X(name, val)                                              \
    case val:                                                     \
        LOG_DEBUG(0, INDENT "Event: VAPPTF_SCE_BT_EVENT_" #name); \
        break;
        VAPPTF_SCE_BT_EVENT_LIST(X)
#undef X
        default:
            break;
    }
#endif  // NDEBUG
}

/**
 * Bluetooth event callback for a batch of one or more events.
 *
 * @return Success always.
 */
static int event_callback(int notifyId, int notifyCount, int notifyArg, void* userData) {
    (void)notifyId;
    (void)notifyCount;
    (void)notifyArg;
    (void)userData;

    while (true) {
        SceBtEvent event = {};

        // Fetch event data.
        int ret = ksceBtReadEvent(&event, 1);

        // Handle errors.
        if (ret == SCE_BT_ERROR_CB_OVERFLOW) {
            LOG_WARN("ksceBtReadEvent reported dropped events");
            handle_event_dropped();
            continue;
        }
        if (ret < 0) {
            LOG_ERROR("ksceBtReadEvent returned error 0x%08X", ret);
            break;
        }

        // Handle no more events to read.
        if (ret == 0) {
            break;
        }

        // Continue in handler.
        handle_event(&event);
    }

    return 0;
}

/**
 * Event thread that creates and registers a callback for bluetooth events.
 *
 * The thread waits for and runs callbacks until run_thread signals it to stop.
 *
 * @return Success always.
 */
static int event_thread(SceSize args, void* argp) {
    (void)args;
    (void)argp;

    LOG_DEBUG(0, "Thread started");

    // Create callback.
    uid_callback = ksceKernelCreateCallback("kvqmbt-bt_event-event_callback", 0, event_callback, NULL);
    LOG_DEBUG(0, "ksceKernelCreateCallback returned 0x%08X", uid_callback);

    // Register callback.
    const unsigned int id_mask = ~(
        // Ignore irrelevant IDs. Set to 0xFFFFFFFF to receive and log all events.
        (1U << VAPPTF_SCE_BT_EVENT_INQUIRY_RESULT) | (1U << VAPPTF_SCE_BT_EVENT_INQUIRY_STOP));
    int ret = ksceBtRegisterCallback(uid_callback, 0, id_mask, 0);
    LOG_DEBUG(0, "ksceBtRegisterCallback returned 0x%08X", ret);

    // Run until thread is stopped.
    while (__atomic_load_n(&run_thread, __ATOMIC_SEQ_CST)) {
        ksceKernelDelayThreadCB(200 * 1000);  // Callback called in here.
    }

    // Thread is stopping, clean up.
    ret = ksceBtUnregisterCallback(uid_callback);
    LOG_DEBUG(0, "ksceBtUnregisterCallback returned 0x%08X", ret);
    ret = ksceKernelDeleteCallback(uid_callback);
    LOG_DEBUG(0, "ksceKernelDeleteCallback returned 0x%08X", ret);
    uid_callback = -1;

    LOG_DEBUG(0, "Thread exiting");

    return 0;
}

/**
 * Create a thread to handle bluetooth events and start it.
 *
 * @return 0 on success, negative on error.
 */
int bt_event_start(void) {
    if (uid_thread >= 0) {
        return 0;
    }

    __atomic_store_n(&run_thread, true, __ATOMIC_SEQ_CST);

    // Create the thread.
    uid_thread = ksceKernelCreateThread("kvqmbt-bt_event-event_thread", event_thread, THREAD_PRIORITY, THREAD_STACK_SIZE,
                                        0, SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT, NULL);
    if (uid_thread < 0) {
        LOG_ERROR("ksceKernelCreateThread returned error 0x%08X", uid_thread);
        return VAPPTF_ERROR_KERNEL_SIDE;
    }
    LOG_DEBUG(0, "ksceKernelCreateThread returned 0x%08X", uid_thread);

    // Start the thread.
    int ret = ksceKernelStartThread(uid_thread, 0, NULL);
    LOG_DEBUG(0, "ksceKernelStartThread returned 0x%08X", ret);

    return 0;
}

/**
 * Shut down the running event-handling thread.
 *
 * @return 0 on success, negative on error.
 */
int bt_event_stop(void) {
    if (uid_thread < 0) {
        return 0;
    }

    // Tell the thread to stop.
    __atomic_store_n(&run_thread, false, __ATOMIC_SEQ_CST);

    // Wait for thread to stop.
    int ret = ksceKernelWaitThreadEnd(uid_thread, NULL, NULL);
    LOG_DEBUG(0, "ksceKernelWaitThreadEnd returned 0x%08X", ret);

    // Delete the thread.
    ret = ksceKernelDeleteThread(uid_thread);
    LOG_DEBUG(0, "ksceKernelDeleteThread returned 0x%08X", ret);

    uid_thread = -1;

    return 0;
}
