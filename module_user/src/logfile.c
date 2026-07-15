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
 * @brief Module-specific functions for log files.
 ******************************************************************************/

#include "logfile.h"

#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdbool.h>

#include "log.h"

static bool is_initialized = false;

/**
 * Initialize the log file dependencies.
 */
void logfile_init(void) {
#ifndef NDEBUG
    int ret = 0;

    // Create log directories.
    ret = sceIoMkdir(LOGFILE_DIR_PARENT, 0777);
    if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
        LOG_ERROR("sceIoMkdir(log_dir=\"%s\") returned error 0x%08X", LOGFILE_DIR_PARENT, ret);
        return;
    }
    ret = sceIoMkdir(LOGFILE_DIR, 0777);
    if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
        LOG_ERROR("sceIoMkdir(log_dir=\"%s\") returned error 0x%08X", LOGFILE_DIR, ret);
        return;
    }

    is_initialized = true;
#endif  // NDEBUG
}

/**
 * Write a log statement to the log file.
 *
 * TODO:
 * - Optimize
 *
 * @param y Current year (for the filename).
 * @param m Current month (for the filename).
 * @param d Current day (for the filename).
 * @param line Log statement format string (supports %s and other format specifiers).
 * @param ... Arguments for the format specifiers.
 */
void logfile_write_line(int y, int m, int d, const char* line, ...) {
    if (!is_initialized) {
        return;
    }
    int ret = 0;

    // Determine filename.
    char log_file_path[256];
    ret = sceClibSnprintf(log_file_path, sizeof(log_file_path), LOGFILE_DIR LOGFILE_FILENAME_FORMAT, y, m, d);
    if (ret < 0) {
        is_initialized = false;
        LOG_ERROR("sceClibSnprintf returned error 0x%08X", ret);
        return;
    }
    if ((size_t)ret >= sizeof(log_file_path)) {
        is_initialized = false;
        LOG_ERROR("sceClibSnprintf truncated the file path");
        return;
    }

    // Format line.
    char buffer[LOGFILE_LINE_BUFFER];
    va_list args;
    va_start(args, line);
    ret = sceClibVsnprintf(buffer, sizeof(buffer), line, args);
    va_end(args);  // NOLINT(clang-analyzer-security.VAList)
    if (ret < 0) {
        LOG_ERROR("sceClibVsnprintf returned error 0x%08X", ret);
        return;
    }
    int buffer_len = ret;
    if ((size_t)buffer_len >= sizeof(buffer)) {
        // Truncated
        static const char marker[] = " Truncated\n";
        const size_t marker_len = sizeof(marker) - 1;  // exclude NUL
        buffer_len = sizeof(buffer) - 1;
        sceClibMemcpy(&buffer[buffer_len - marker_len], marker, marker_len);
    }

    // Write to log file.
    SceUID fd = sceIoOpen(log_file_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd < 0) {
        is_initialized = false;
        LOG_ERROR("sceIoOpen(log_file_path=\"%s\") returned error 0x%08X", log_file_path, fd);
        return;
    }
    sceIoWrite(fd, buffer, buffer_len);
    sceIoClose(fd);
}
