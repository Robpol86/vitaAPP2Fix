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
 * Resolve ksceKernelGetModuleInfo at runtime via module_get_export_func.
 * Must be called once from module_start before dump_module().
 *
 * @return 0 on success, negative on error.
 */
int dump_module_init(void);

/**
 * Dump every loadable segment of a running kernel module to ux0:.
 *
 * Output: ux0:<PROJECT_NAME>/dumps/<module_name>_seg<N>_0x<vaddr>.bin
 * Use the vaddr in the filename as Ghidra's image base for that segment.
 *
 * @param module_name  Module name as known to taiHEN, e.g. "SceBt".
 * @param expected_nid Module NID to verify (0 = skip check). If the live
 *                     module's NID differs, the dump aborts -- offsets derived
 *                     from a mismatched build would cause panics at hook time.
 * @return 0 on success, negative on error.
 */
int dump_module(const char* module_name, unsigned int expected_nid);

#endif  // DUMP_MODULE_H
