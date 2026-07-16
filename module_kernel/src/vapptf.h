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
 * @brief Cross-module interface exposed by the kernel module.
 ******************************************************************************/

#ifndef VAPPTF_H
#define VAPPTF_H

// Error codes.
typedef enum VapptfError : int {
    VAPPTF_ERROR_INVALID_ARGUMENT = (int)0x80680001,  // Arbitrary first value.
    VAPPTF_ERROR_KERNEL_SIDE,
    VAPPTF_ERROR_KERNEL_SIDE_NOT_CONNECTABLE,
    VAPPTF_ERROR_KERNEL_SIDE_BUSY,
    VAPPTF_ERROR_CB_OVERFLOW,
    VAPPTF_ERROR_NOT_READY,
    VAPPTF_ERROR_GENERAL_FAILURE,
    VAPPTF_ERROR_KERNEL_SIDE_ALREADY_CONNECTED,
} VapptfError;

#endif  // VAPPTF_H
