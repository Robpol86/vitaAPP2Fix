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
 * @brief Main entrypoint of the plugin.
 ******************************************************************************/

#include <psp2/kernel/modulemgr.h>
#include <stdbool.h>

#include "log.h"
#include "logfile.h"
#include "quickmenu.h"

/**
 * Main entrypoint. Called when the module is started.
 *
 * @param args The size of the arguments passed to the module.
 * @param argp A pointer to the arguments passed to the module.
 * @return SCE_KERNEL_START_SUCCESS on success, or an error code on failure.
 */
int module_start(SceSize args, const void* argp) {
    (void)args;
    (void)argp;

    // Initialize logging.
    logfile_init();
    LOG_INFO("Starting");

    // Start quickmenu routines.
    int ret = quickmenu_start();
    if (ret < 0) {
        LOG_ERROR("quickmenu_start returned error 0x%08X", ret);
        return SCE_KERNEL_START_FAILED;
    }

    LOG_INFO("Started");

    return SCE_KERNEL_START_SUCCESS;
}

/**
 * Unloading entrypoint. Called when the module is stopped.
 *
 * @param args The size of the arguments passed to the module.
 * @param argp A pointer to the arguments passed to the module.
 * @return SCE_KERNEL_STOP_SUCCESS on success, or an error code on failure.
 */
int module_stop(SceSize args, const void* argp) {
    (void)args;
    (void)argp;

    bool failed = false;

    LOG_INFO("Stopping");

    // Stop quickmenu routines.
    int ret = quickmenu_stop();
    if (ret < 0) {
        LOG_ERROR("quickmenu_stop returned error 0x%08X", ret);
        failed = true;
    }

    LOG_INFO("Stopped");

    // TODO drop (int) cast on new clang-tidy: https://github.com/llvm/llvm-project/issues/195604
    return (int)failed ? SCE_KERNEL_STOP_FAIL : SCE_KERNEL_STOP_SUCCESS;
}
