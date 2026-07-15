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
 * @brief Module-agnostic macros for logging.
 ******************************************************************************/

#ifndef LOG_H
#define LOG_H

#ifndef NDEBUG
#define LOG_COLOR_RESET_ "\033[0m"
#define LOG_COLOR_BOLD_ "\033[1m"
#define LOG_COLOR_RED_ "\033[1;91m"
#define LOG_COLOR_CYAN_ "\033[1;96m"
#define LOG_COLOR_YELLOW_ "\033[1;93m"
#define LOG_COLOR_GRAY_ "\033[90m"
#else
#define LOG_COLOR_RESET_ ""
#define LOG_COLOR_BOLD_ ""
#define LOG_COLOR_RED_ ""
#define LOG_COLOR_CYAN_ ""
#define LOG_COLOR_YELLOW_ ""
#define LOG_COLOR_GRAY_ ""
#endif  // NDEBUG

#define LOG_FORMAT_TIME_ LOG_COLOR_GRAY_ "[%02d:%02d:%02d.%03d]" LOG_COLOR_RESET_
#define LOG_FORMAT_ORIGIN_ LOG_COLOR_BOLD_ "%s:%d:%s" LOG_COLOR_RESET_
#define LOG_FORMAT_(category) LOG_FORMAT_TIME_ " [" category "] [" MODULE_NAME "] [0x%08X] [" LOG_FORMAT_ORIGIN_ "] "
#define LOG_FORMAT_VALUES_(dt, tid) \
    (dt).hour, (dt).minute, (dt).second, (dt).microsecond / 1000, tid, __FILE__, __LINE__, __func__

/**
 * Macro that logs an error message.
 *
 * @param msg The log message (supports %s and other format specifiers).
 * @param ... Arguments for the format specifiers.
 */
#define LOG_ERROR(msg, ...) LOG_MESSAGE_(0, LOG_FORMAT_(LOG_COLOR_RED_ "ERROR" LOG_COLOR_RESET_), msg, ##__VA_ARGS__)

/**
 * Macro that logs an info message.
 *
 * @param msg The log message (supports %s and other format specifiers).
 * @param ... Arguments for the format specifiers.
 */
#define LOG_INFO(msg, ...) LOG_MESSAGE_(0, LOG_FORMAT_(LOG_COLOR_CYAN_ "INFO " LOG_COLOR_RESET_), msg, ##__VA_ARGS__)

/**
 * Macro that logs a warning message.
 *
 * @param msg The log message (supports %s and other format specifiers).
 * @param ... Arguments for the format specifiers.
 */
#define LOG_WARN(msg, ...) LOG_MESSAGE_(0, LOG_FORMAT_(LOG_COLOR_YELLOW_ "WARN " LOG_COLOR_RESET_), msg, ##__VA_ARGS__)

#ifndef NDEBUG
/**
 * Macro that logs a debug message.
 *
 * If log messages overwrite each other try setting the delay to 50000.
 *
 * @param delay Delay the thread for these many microseconds if >0 (mitigates clobbering).
 * @param msg The log message (supports %s and other format specifiers).
 * @param ... Arguments for the format specifiers.
 */
#define LOG_DEBUG(delay, msg, ...) LOG_MESSAGE_(delay, LOG_FORMAT_("DEBUG"), msg, ##__VA_ARGS__)
#else
#define LOG_DEBUG(delay, msg, ...) LOG_MESSAGE_NOOP_(delay, "", msg, ##__VA_ARGS__)
#endif  // NDEBUG

#endif  // LOG_H
