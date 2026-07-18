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
 * @brief TIER 1: hook the exported SceBt audio-path functions.
 *
 * These are the A2DP media entry points. They are *above* the AVDTP codec
 * negotiation (that has already happened by the time StartAudio fires), so
 * this will not print the codec directly. What it WILL tell you, with zero
 * reverse engineering, is:
 *
 *   1. Does the Vita even call StartAudio/SendAudio for the APP2? If those
 *      never fire (or FreqAudio returns an error) the stream setup died in
 *      negotiation and you go straight to Tier 2.
 *   2. Do the scalar args differ between APP1 and APP2? Capture a session of
 *      each and diff the logs. A differing rate/handle/flag arg is a strong
 *      lead.
 *
 * NOTE: r0..r3 semantics are unknown (see psp2kern/bt.h). This logs them as
 * scalars only. It deliberately does NOT dereference them as pointers, because
 * a wrong guess is an instant kernel panic. Once you see which args are stable
 * small integers (handles/rates) vs large aligned values (likely pointers),
 * you can extend this to dump a buffer -- carefully, with a bounds check.
 ******************************************************************************/

#include "bt_audio_hook.h"

#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

#include "log.h"

// SceBtForDriver NIDs (firmware 3.60/3.65, from vita-headers db/360/SceBt.yml).
#define NID_ksceBtStartAudio 0x8D47CABD
#define NID_ksceBtStopAudio 0xCAE5A9F3
#define NID_ksceBtSendAudio 0x47F19727
#define NID_ksceBtFreqAudio 0xDA20DCC8

// TAI_CONTINUE expands to a call through `type(*)()`. In C that empty
// parameter list means "unspecified args" and accepts our four ints; in C++
// it means "zero args" and hard-errors. This helper replicates the macro's
// hook-chain walk (call the next hook if present, else the original) through a
// correctly-typed function pointer, so it works in a .cpp TU.
typedef int (*bt_audio_fn_t)(int, int, int, int);
static inline int bt_continue(tai_hook_ref_t ref, int r0, int r1, int r2, int r3) {
    struct _tai_hook_user* cur = (struct _tai_hook_user*)ref;
    struct _tai_hook_user* next = (struct _tai_hook_user*)cur->next;
    bt_audio_fn_t fn = (bt_audio_fn_t)(next == NULL ? cur->old : next->func);
    return fn(r0, r1, r2, r3);
}

static tai_hook_ref_t ref_start = 0;
static tai_hook_ref_t ref_stop = 0;
static tai_hook_ref_t ref_send = 0;
static tai_hook_ref_t ref_freq = 0;

static SceUID uid_start = -1;
static SceUID uid_stop = -1;
static SceUID uid_send = -1;
static SceUID uid_freq = -1;

// SendAudio packet counter. Reset by bt_audio_hook_reset_counter() -- typically
// called from the TOGGLE_BLUETOOTH event handler so each BT power-on gives a
// fresh first-N packets in the log.
static int send_count = 0;

// StartAudio: fires when the Vita opens/starts the A2DP media stream. If this
// is absent for the APP2, negotiation failed before streaming -> Tier 2.
static int hook_start_audio(int r0, int r1, int r2, int r3) {
    LOG_DEBUG(0, "ksceBtStartAudio(r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X)", r0, r1, r2, r3);
    int ret = bt_continue(ref_start, r0, r1, r2, r3);
    LOG_DEBUG(0, "ksceBtStartAudio -> 0x%08X", ret);
    return ret;
}

// FreqAudio: often carries the sample rate / stream tuning. A differing arg or
// a nonzero return between APP1 and APP2 is a prime suspect.
static int hook_freq_audio(int r0, int r1, int r2, int r3) {
    LOG_DEBUG(0, "ksceBtFreqAudio(r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X)", r0, r1, r2, r3);
    int ret = bt_continue(ref_freq, r0, r1, r2, r3);
    LOG_DEBUG(0, "ksceBtFreqAudio -> 0x%08X", ret);
    return ret;
}

static int hook_stop_audio(int r0, int r1, int r2, int r3) {
    LOG_DEBUG(0, "ksceBtStopAudio(r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X)", r0, r1, r2, r3);
    return bt_continue(ref_stop, r0, r1, r2, r3);
}

// SendAudio fires per media packet -- VERY chatty. Left rate-limited to the
// first few calls so you can confirm packets flow without flooding the log.
// The counter is a file-scope int reset by bt_audio_hook_reset_counter().
static int hook_send_audio(int r0, int r1, int r2, int r3) {
    if (send_count < 8) {
        LOG_DEBUG(0, "ksceBtSendAudio(r0=0x%08X r1=0x%08X r2=0x%08X r3=0x%08X) #%d", r0, r1, r2, r3, send_count);
        send_count++;
    }
    return bt_continue(ref_send, r0, r1, r2, r3);
}

// Idempotent: if hooks are already installed, just resets the counter. This
// makes it safe to invoke on every TOGGLE_BLUETOOTH event without worrying
// about whether the event fires only on off->on or on both transitions.
int bt_audio_hook_start(void) {
    send_count = 0;

    if (uid_start >= 0 || uid_freq >= 0 || uid_stop >= 0 || uid_send >= 0) {
        LOG_DEBUG(0, "Hooks already installed, counter reset only");
        return 0;
    }
    LOG_DEBUG(0, "STARTING EXPERIMENT");

    uid_start = taiHookFunctionExportForKernel(KERNEL_PID, &ref_start, "SceBt", TAI_ANY_LIBRARY, NID_ksceBtStartAudio,
                                               (const void*)hook_start_audio);
    LOG_DEBUG(0, "hook StartAudio -> 0x%08X", uid_start);

    uid_freq = taiHookFunctionExportForKernel(KERNEL_PID, &ref_freq, "SceBt", TAI_ANY_LIBRARY, NID_ksceBtFreqAudio,
                                              (const void*)hook_freq_audio);
    LOG_DEBUG(0, "hook FreqAudio -> 0x%08X", uid_freq);

    uid_stop = taiHookFunctionExportForKernel(KERNEL_PID, &ref_stop, "SceBt", TAI_ANY_LIBRARY, NID_ksceBtStopAudio,
                                              (const void*)hook_stop_audio);
    LOG_DEBUG(0, "hook StopAudio -> 0x%08X", uid_stop);

    uid_send = taiHookFunctionExportForKernel(KERNEL_PID, &ref_send, "SceBt", TAI_ANY_LIBRARY, NID_ksceBtSendAudio,
                                              (const void*)hook_send_audio);
    LOG_DEBUG(0, "hook SendAudio -> 0x%08X", uid_send);

    return 0;
}

// Idempotent: safe to call regardless of hook state.
int bt_audio_hook_stop(void) {
    LOG_DEBUG(0, "STOPPING EXPERIMENT");
    if (uid_send >= 0) taiHookReleaseForKernel(uid_send, ref_send), uid_send = -1;
    if (uid_stop >= 0) taiHookReleaseForKernel(uid_stop, ref_stop), uid_stop = -1;
    if (uid_freq >= 0) taiHookReleaseForKernel(uid_freq, ref_freq), uid_freq = -1;
    if (uid_start >= 0) taiHookReleaseForKernel(uid_start, ref_start), uid_start = -1;
    return 0;
}
