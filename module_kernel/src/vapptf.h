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
 * @brief Misc declarations for the project.
 ******************************************************************************/

#ifndef VAPPTF_H
#define VAPPTF_H

// Error codes.
typedef enum VapptfError : int {
    VAPPTF_ERROR_KERNEL_SIDE = (int)0x80680001,  // Arbitrary first value.
} VapptfError;

#endif  // VAPPTF_H
