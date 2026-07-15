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
 * @brief Module-specific macros for log files.
 ******************************************************************************/

#ifndef LOGFILE_H_MODULE
#define LOGFILE_H_MODULE

#include <logfile.h>

void logfile_init(void);
void logfile_write_line(int y, int m, int d, const char* line, ...) __attribute__((format(printf, 4, 5)));

#endif  // LOGFILE_H_MODULE
