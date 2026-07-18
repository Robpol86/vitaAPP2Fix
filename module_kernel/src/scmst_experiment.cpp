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
 * SceBt.elf reverse engineering confirmed the Vita's A2DP source SEP advertises
 * SCMS-T content protection (AVDTP Content Protection capability, CP_TYPE
 * 0x0002) alongside its single SBC MEDIA_CODEC capability. AirPods Pro 2's
 * "connects but no media audio" behaviour is the classic SCMS-T mismatch
 * signature. Toggling SCMS-T off at driver level is the cheapest experiment
 * to test that hypothesis.
 *
 * ksceBtSetContentProtection is declared in psp2kern/bt.h as:
 *   int ksceBtSetContentProtection(int r0);
 *
 * The argument's semantics are not documented in the header. Convention on the
 * Vita for setter-with-boolean is 0 = disable, 1 = enable, so we pass 0 here.
 * If APP2 audio starts working after this call, the hypothesis is confirmed
 * (and you'll want to think about whether to leave SCMS-T off system-wide or
 * hook something narrower). If audio still fails, SCMS-T is exonerated and we
 * move to hypothesis 2 (SEP selection).
 *
 * Bounds on interpretation:
 *   - A negative return means the call failed outright; SCMS-T state is
 *     unchanged and this experiment is inconclusive.
 *   - A zero/positive return means the call succeeded, but does NOT prove that
 *     the setting is honoured in the next A2DP negotiation. The definitive
 *     signal is: does audio come out of APP2 after this + reconnect?
 *   - This must be called BEFORE APP2 connects. If APP2 is already paired and
 *     connected, disconnect and reconnect it after the module loads for the
 *     new CP setting to take effect during SEP negotiation.
 ******************************************************************************/

#include "scmst_experiment.h"

#include <psp2kern/bt.h>

#include "log.h"

int scmst_experiment_run(void) {
    LOG_INFO("SCMS-T experiment: calling ksceBtSetContentProtection(0) to disable");

    int ret = ksceBtSetContentProtection(0);

    if (ret < 0) {
        LOG_ERROR("ksceBtSetContentProtection failed: 0x%08X", ret);  // SCMS-T state unchanged, inconclusive
    } else {
        LOG_INFO("ksceBtSetContentProtection returned 0x%08X", ret);  // Reconnect APP2 and test audio
    }

    return ret;
}
