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
 *
 * SceBt.elf reverse engineering (3.65) confirmed the Vita's A2DP source SEP
 * advertises SCMS-T (AVDTP Content Protection cap, CP_TYPE 0x0002) alongside
 * a single SBC MEDIA_CODEC cap. The APP2 "connects but no media audio"
 * symptom is a classic SCMS-T mismatch signature, so ksceBtSetContentProtection
 * is worth probing before more expensive work.
 *
 * The header (psp2kern/bt.h) declares:
 *   int ksceBtSetContentProtection(int r0);
 * and does not document r0's semantics. Convention on the Vita is usually
 * 0 = disable / 1 = enable, but not always. Distinguish empirically by
 * running with arg=1 first and observing APP1 (the working control):
 *   - APP1 loses audio when arg=1 -> 1 = enable, 0 = disable (SCMS-T ruled
 *     out by an earlier arg=0 run that left APP2 silent).
 *   - APP1 still works when arg=1 -> 1 also doesn't affect SCMS-T. Either
 *     the setting is per-connection and both endpoints already handshook, or
 *     this API doesn't gate the AVDTP capability we saw in the ELF. Both
 *     interpretations mean this experiment can't rule SCMS-T in or out on
 *     its own; other evidence is needed.
 ******************************************************************************/

#include "scmst_experiment.h"

#include <psp2kern/bt.h>

#include "log.h"

int scmst_experiment_run(int arg) {
    LOG_INFO("SCMS-T experiment: calling ksceBtSetContentProtection(%d)", arg);

    int ret = ksceBtSetContentProtection(arg);

    if (ret < 0) {
        LOG_ERROR("ksceBtSetContentProtection(%d) failed: 0x%08X", arg, ret);  // SCMS-T state unchanged, inconclusive
    } else {
        LOG_INFO("ksceBtSetContentProtection(%d) returned 0x%08X", arg, ret);  // Reconnect APP2 and test audio
    }

    return ret;
}
