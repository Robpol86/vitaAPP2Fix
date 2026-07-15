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
 * @brief Module-agnostic macros for log files.
 ******************************************************************************/

#ifndef LOGFILE_H
#define LOGFILE_H

#define SCE_ERROR_ERRNO_EEXIST 0x80010011

#define LOGFILE_DIR_PARENT "ux0:" PROJECT_NAME
#define LOGFILE_DIR LOGFILE_DIR_PARENT "/logs/"
#define LOGFILE_FILENAME_FORMAT PROJECT_NAME "-%04d%02d%02d.log"

#define LOGFILE_LINE_BUFFER 512

#ifndef NDEBUG
#define LOG_MESSAGE_TO_FILE(y, m, d, fmt, ...) logfile_write_line(y, m, d, fmt, ##__VA_ARGS__)
#else
#define LOG_MESSAGE_TO_FILE(y, m, d, fmt, ...) ((void)0)
#endif  // NDEBUG

#endif  // LOGFILE_H
