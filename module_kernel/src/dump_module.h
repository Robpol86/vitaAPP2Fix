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
 * @brief Dump a loaded kernel module's segments to ux0: for offline RE.
 ******************************************************************************/

#ifndef DUMP_MODULE_H
#define DUMP_MODULE_H

/**
 * Dump every loadable segment of a loaded kernel module to ux0:.
 *
 * The dump is taken from live kernel memory, so it is already decrypted and
 * its addresses match exactly what taiGetModuleInfoForKernel /
 * taiHookFunctionOffsetForKernel will use at runtime -- no SELF decryption and
 * no 3.60-vs-3.65 offset mismatch.
 *
 * @param module_name  Module name, e.g. "SceBt".
 * @param expected_nid Module NID to verify against (0 to skip the check).
 *                     If the live module's NID differs, the dump is aborted,
 *                     because any offset you derive from it would not match.
 * @return 0 on success, negative on error.
 */
int dump_module(const char* module_name, unsigned int expected_nid);

#endif  // DUMP_MODULE_H
