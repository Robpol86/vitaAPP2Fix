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
 * @brief Patch SceBt's SBC source config to negotiate 44.1kHz instead of 48kHz.
 ******************************************************************************/

#ifndef SBC_FREQ_PATCH_H
#define SBC_FREQ_PATCH_H

int sbc_freq_patch_start(void);
int sbc_freq_patch_stop(void);

#endif  // SBC_FREQ_PATCH_H
