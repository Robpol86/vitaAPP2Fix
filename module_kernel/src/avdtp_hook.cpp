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
 * @brief Offset hook on SceBt's internal AVDTP signaling handler.
 *
 * Reverse engineering of SceBt.elf (3.65, module NID 0x67E0C2EB) located the
 * AVDTP signaling PDU handler at seg0 offset 0x9F4C (VA 0x81009F4C). Its
 * prologue reads PDU byte 0 (transaction label / packet type / message type),
 * masks PDU byte 1 to a 6-bit signal ID, and bounds-checks it into a dispatch
 * table -- the canonical shape of an AVDTP signaling entry point.
 *
 * Signature recovered from the prologue register moves:
 *   int handler(void *ctx, int a1, const unsigned char *pdu, int pdu_len);
 *     r0 = ctx  (per-connection context)
 *     r1 = a1   (unused by us)
 *     r2 = pdu  (AVDTP signaling PDU, byte 0 = header)
 *     r3 = pdu_len
 *
 * We hook by offset (not export) because the handler is internal and invoked
 * indirectly. The hook feeds each PDU to avdtp_parse(), which decodes the
 * Media Codec capability -- letting us see exactly which SBC parameters get
 * negotiated (SET_CONFIGURATION) with APP1 vs APP2.
 *
 * CMakeLists: link taihenForKernel_stub and taihenModuleUtils_stub (already
 * linked for the audio hooks / dumper). No new stubs required.
 ******************************************************************************/

#include "avdtp_hook.h"

#include <taihen.h>

#include "avdtp_parse.h"
#include "log.h"

// SceBt module identity (3.65). If the NID ever mismatches, the offset is wrong
// for the running build and we must NOT hook (guards against panics).
#define SCEBT_MODULE_NAME "SceBt"
#define SCEBT_EXPECTED_NID 0x67E0C2EB

// AVDTP signaling handler location within SceBt seg0 (from RE).
#define AVDTP_HANDLER_SEGIDX 0
#define AVDTP_HANDLER_OFFSET 0x9F4C
#define AVDTP_HANDLER_THUMB 1

static tai_hook_ref_t s_ref = 0;
static SceUID s_uid = -1;

// Typed continue helper (TAI_CONTINUE macro is C-only; see bt_audio_hook.cpp).
typedef int (*avdtp_fn_t)(void*, int, const unsigned char*, int);
static inline int avdtp_continue(void* ctx, int a1, const unsigned char* pdu, int len) {
    struct _tai_hook_user* cur = (struct _tai_hook_user*)s_ref;
    struct _tai_hook_user* next = (struct _tai_hook_user*)cur->next;
    avdtp_fn_t fn = (avdtp_fn_t)(next == NULL ? cur->old : next->func);
    return fn(ctx, a1, pdu, len);
}

static int hook_avdtp(void* ctx, int a1, const unsigned char* pdu, int len) {
    // Decode the incoming PDU before passing it on. avdtp_parse is defensive
    // about length, but guard the pointer/length here too.
    if (pdu != NULL && len >= 2 && len < 0x1000) {
        avdtp_parse(pdu, (unsigned int)len);
    }
    return avdtp_continue(ctx, a1, pdu, len);
}

int avdtp_hook_start(void) {
    if (s_uid >= 0) {
        LOG_DEBUG(0, "avdtp_hook already installed");
        return 0;
    }

    // Verify we're hooking the build these offsets came from.
    tai_module_info_t tinfo;
    tinfo.size = sizeof(tinfo);
    int ret = taiGetModuleInfoForKernel(KERNEL_PID, SCEBT_MODULE_NAME, &tinfo);
    if (ret < 0) {
        LOG_ERROR("avdtp_hook: taiGetModuleInfoForKernel = 0x%08X", ret);
        return ret;
    }
    if (tinfo.module_nid != SCEBT_EXPECTED_NID) {
        // refusing to hook to avoid a panic.
        LOG_WARN("avdtp_hook: SceBt NID 0x%08X != expected 0x%08X. Offset 0x%X is for a different build",
                 tinfo.module_nid, SCEBT_EXPECTED_NID, AVDTP_HANDLER_OFFSET);
        return -1;
    }

    s_uid = taiHookFunctionOffsetForKernel(KERNEL_PID, &s_ref, tinfo.modid, AVDTP_HANDLER_SEGIDX, AVDTP_HANDLER_OFFSET,
                                           AVDTP_HANDLER_THUMB, (const void*)hook_avdtp);
    LOG_DEBUG(0, "avdtp_hook: hook @ seg%d+0x%X -> 0x%08X", AVDTP_HANDLER_SEGIDX, AVDTP_HANDLER_OFFSET, s_uid);
    if (s_uid < 0) {
        int err = s_uid;
        s_uid = -1;
        return err;
    }
    return 0;
}

int avdtp_hook_stop(void) {
    if (s_uid >= 0) {
        taiHookReleaseForKernel(s_uid, s_ref);
        s_uid = -1;
    }
    return 0;
}
