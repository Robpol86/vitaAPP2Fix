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
 * @brief Offset hook on SceBt's internal AVDTP signaling handler, to log the
 *        negotiated codec configuration per device.
 ******************************************************************************/

#ifndef AVDTP_HOOK_H
#define AVDTP_HOOK_H

int avdtp_hook_start(void);
int avdtp_hook_stop(void);

#endif  // AVDTP_HOOK_H
