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
 * @brief Module-specific macros for logging.
 ******************************************************************************/

#ifndef LOG_H_MODULE
#define LOG_H_MODULE

#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/rtc.h>

#include "logfile.h"  // IWYU pragma: keep

/**
 * Main macro that handles getting the current time, logging to stdout, logging
 * to log file via LOG_MESSAGE_TO_FILE macro, then optionally sleeping the thread.
 */
#define LOG_MESSAGE_(delay, fmt, msg, ...)                                                                             \
    do {                                                                                                               \
        SceDateTime dt_;                                                                                               \
        sceRtcGetCurrentClockUtc(&dt_);                                                                                \
        int tid_ = sceKernelGetThreadId();                                                                             \
        sceClibPrintf(fmt msg "\n", LOG_FORMAT_VALUES_(dt_, tid_), ##__VA_ARGS__);                                     \
        LOG_MESSAGE_TO_FILE(dt_.year, dt_.month, dt_.day, fmt msg "\n", LOG_FORMAT_VALUES_(dt_, tid_), ##__VA_ARGS__); \
        if (!__builtin_constant_p(delay) || (delay) > 0) sceKernelDelayThread((delay));                                \
    } while (0)

/**
 * Macro that does nothing, used by include/log.h when logging should be disabled.
 */
#define LOG_MESSAGE_NOOP_(delay, fmt, msg, ...)       \
    do {                                              \
        (void)(delay);                                \
        if (0) sceClibPrintf(fmt msg, ##__VA_ARGS__); \
    } while (0)

#include <log.h>  // Bring in LOG_DEBUG, LOG_ERROR, and other common macros.

#endif  // LOG_H_MODULE
