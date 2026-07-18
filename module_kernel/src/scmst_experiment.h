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
 * @brief SCMS-T toggle experiment for APP2 no-audio investigation.
 ******************************************************************************/

#ifndef SCMST_EXPERIMENT_H
#define SCMST_EXPERIMENT_H

/**
 * Calls ksceBtSetContentProtection(arg) and logs the outcome. The semantics of
 * arg (0 vs 1 = disable vs enable) are not documented in the header; run once
 * with each value and observe APP1 (control) behaviour to determine which
 * value disables SCMS-T.
 *
 * @param arg  Value to pass to ksceBtSetContentProtection.
 * @return     The value returned by ksceBtSetContentProtection.
 */
int scmst_experiment_run(int arg);

#endif  // SCMST_EXPERIMENT_H
