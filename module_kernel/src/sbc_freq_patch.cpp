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
 *
 * Reverse engineering of SceBt.elf (3.65, module NID 0x67E0C2EB) found the SBC
 * source-SEP config template at seg0 offset 0x1F2D2 (VA 0x8101F2D2):
 *
 *     07 06 00 00 19 F5 02 33
 *     |  |  |  |  |  |  |  +-- max bitpool (0x33 = 51)
 *     |  |  |  |  |  |  +----- min bitpool (0x02)
 *     |  |  |  |  |  +-------- block/subbands/allocation (0xF5)
 *     |  |  |  |  +----------- [sampling_freq:4][channel_mode:4] = 0x19
 *     |  |  |  +-------------- codec type (0x00 = SBC)
 *     |  |  +----------------- media type (0x00 = audio)
 *     |  +-------------------- LOSC (0x06)
 *     +----------------------- Media Codec category (0x07)
 *
 * The sampling-frequency nibble is 0x1 = 48kHz. AirPods H2-generation buds
 * (APP2, APP3) accept SET_CONFIGURATION and reach START but render no audio,
 * while H1 (APP1) plays fine -- everything else in the Vita pipeline is
 * byte-identical between them. This patch flips that nibble to 0x2 = 44.1kHz
 * (the byte 0x19 -> 0x29) to test whether the H2 decoder mishandles 48kHz SBC.
 * The AirPods advertise both 44.1 and 48kHz in their SBC caps, so 44.1kHz will
 * negotiate cleanly.
 *
 * Reversible: taiInjectDataForKernel snapshots the original byte and restores it
 * on taiInjectReleaseForKernel (called from sbc_freq_patch_stop).
 *
 * CAVEAT: ksceBtSendAudio is fed 48kHz PCM upstream of the SBC encoder. If the
 * encoder does not resample to the newly-negotiated 44.1kHz, audio will play
 * pitch-shifted rather than correct. That is still an informative result -- it
 * proves the config change took effect and the buds render it -- but a proper
 * fix would then also need the source PCM rate changed. Silence after this patch
 * means 48-vs-44.1 was not the cause.
 *
 * CMakeLists: link taihenForKernel_stub (already linked). No new stubs required.
 ******************************************************************************/

#include "sbc_freq_patch.h"

#include <taihen.h>

#include "log.h"

// SceBt module identity (3.65). If the NID mismatches, the offset is wrong for
// the running build and we must NOT patch (guards against corrupting memory).
#define SCEBT_MODULE_NAME "SceBt"
#define SCEBT_EXPECTED_NID 0x67E0C2EB

// SBC source config template: sampling_freq/channel_mode byte (from RE).
#define SBC_CFG_SEGIDX 0
#define SBC_CFG_FREQ_OFFSET 0x1F2D6
#define SBC_CFG_FREQ_48KHZ 0x19  // freq nibble 0x1 = 48kHz  (original)
#define SBC_CFG_FREQ_44KHZ 0x29  // freq nibble 0x2 = 44.1kHz (patched)

static SceUID s_uid = -1;

int sbc_freq_patch_start(void) {
    if (s_uid >= 0) {
        LOG_DEBUG(0, "sbc_freq_patch already applied");
        return 0;
    }

    // Verify we're patching the build these offsets came from.
    tai_module_info_t tinfo;
    tinfo.size = sizeof(tinfo);
    int ret = taiGetModuleInfoForKernel(KERNEL_PID, SCEBT_MODULE_NAME, &tinfo);
    if (ret < 0) {
        LOG_ERROR("sbc_freq_patch: taiGetModuleInfoForKernel = 0x%08X", ret);
        return ret;
    }
    if (tinfo.module_nid != SCEBT_EXPECTED_NID) {
        // refusing to patch to avoid corrupting the wrong build.
        LOG_WARN("sbc_freq_patch: SceBt NID 0x%08X != expected 0x%08X. Offset 0x%X is for a different build",
                 tinfo.module_nid, SCEBT_EXPECTED_NID, SBC_CFG_FREQ_OFFSET);
        return -1;
    }

    const unsigned char patched = SBC_CFG_FREQ_44KHZ;
    s_uid =
        taiInjectDataForKernel(KERNEL_PID, tinfo.modid, SBC_CFG_SEGIDX, SBC_CFG_FREQ_OFFSET, &patched, sizeof(patched));
    LOG_DEBUG(0, "sbc_freq_patch: 48kHz->44.1kHz @ seg%d+0x%X (0x%02X->0x%02X) -> 0x%08X", SBC_CFG_SEGIDX,
              SBC_CFG_FREQ_OFFSET, SBC_CFG_FREQ_48KHZ, SBC_CFG_FREQ_44KHZ, s_uid);
    if (s_uid < 0) {
        int err = s_uid;
        s_uid = -1;
        return err;
    }
    return 0;
}

int sbc_freq_patch_stop(void) {
    if (s_uid >= 0) {
        taiInjectReleaseForKernel(s_uid);
        s_uid = -1;
    }
    return 0;
}
