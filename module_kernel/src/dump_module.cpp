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

#include "dump_module.h"

#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysclib.h>
#include <taihen.h>

#include "log.h"
#include "logfile.h"  // For LOGFILE_DIR_PARENT (ux0:<PROJECT_NAME>).

#define DUMP_DIR LOGFILE_DIR_PARENT "/dumps/"
#define COPY_CHUNK 0x8000

// Bounce buffer: read kernel memory into here, then write to file. Copying via
// memcpy first (a plain kernel->kernel read) avoids any ambiguity about whether
// ksceIoWrite will accept a foreign module's segment address as its source.
static unsigned char g_bounce[COPY_CHUNK];

int dump_module(const char* module_name, unsigned int expected_nid) {
    // Resolve the module to a modid and read its NID.
    tai_module_info_t tinfo;
    tinfo.size = sizeof(tinfo);
    int ret = taiGetModuleInfoForKernel(KERNEL_PID, module_name, &tinfo);
    if (ret < 0) {
        LOG_ERROR("taiGetModuleInfoForKernel(\"%s\") returned 0x%08X", module_name, ret);
        return ret;
    }
    LOG_DEBUG(0, "taiGetModuleInfoForKernel(\"%s\") returned modid=0x%08X nid=0x%08X", module_name, tinfo.modid,
              tinfo.module_nid);

    // Guard against reversing the wrong build.
    if (expected_nid != 0 && tinfo.module_nid != expected_nid) {
        LOG_WARN("nid 0x%08X != expected 0x%08X; offsets from this build would not match. Aborting dump.",
                 tinfo.module_nid, expected_nid);
        return -1;
    }

    // Fetch segment layout.
    SceKernelModuleInfo minfo;
    minfo.size = sizeof(minfo);
    ret = ksceKernelGetModuleInfo(KERNEL_PID, tinfo.modid, &minfo);
    if (ret < 0) {
        LOG_ERROR("ksceKernelGetModuleInfo returned 0x%08X", ret);
        return ret;
    }
    LOG_DEBUG(0, "returned size=0x%08X modid=0x%08X modattr=0x%08X modver=%08X.%08X module_name=\"%s\" path=\"%s\"",
              minfo.size, minfo.modid, minfo.modattr, minfo.modver[0], minfo.modver[1], minfo.module_name, minfo.path);

    ksceIoMkdir(DUMP_DIR, 0777);

    for (int i = 0; i < 4; i++) {
        SceKernelSegmentInfo* seg = &minfo.segments[i];
        if (seg->size == 0 || seg->memsz == 0 || seg->vaddr == NULL) {
            continue;
        }

        LOG_DEBUG(0, "seg[%d] vaddr=0x%08X memsz=0x%08X filesz=0x%08X perms=0x%X", i, (unsigned)(uintptr_t)seg->vaddr,
                  seg->memsz, seg->filesz, seg->perms);

        // Filename encodes the load vaddr so you know the base to use in Ghidra.
        char path[256];
        int n =
            snprintf(path, sizeof(path), DUMP_DIR "%s_seg%d_0x%08X.bin", module_name, i, (unsigned)(uintptr_t)seg->vaddr);
        if (n < 0 || n >= (int)sizeof(path)) {
            LOG_ERROR("snprintf path failed/truncated for seg %d", i);
            continue;
        }

        SceUID fd = ksceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
        if (fd < 0) {
            LOG_ERROR("ksceIoOpen(\"%s\") returned 0x%08X", path, fd);
            continue;
        }

        // Dump memsz bytes (includes zero-init BSS tail, which matches runtime).
        const unsigned char* src = (const unsigned char*)seg->vaddr;
        SceSize offset = 0;
        SceSize remaining = seg->memsz;
        bool ok = true;
        while (remaining > 0) {
            SceSize chunk = remaining > COPY_CHUNK ? COPY_CHUNK : remaining;
            memcpy(g_bounce, src + offset, chunk);
            int w = ksceIoWrite(fd, g_bounce, chunk);
            if (w < 0) {
                LOG_ERROR("ksceIoWrite seg %d at +0x%08X returned 0x%08X", i, offset, w);
                ok = false;
                break;
            }
            offset += (SceSize)w;
            remaining -= (SceSize)w;
        }
        ksceIoClose(fd);
        if (ok) {
            LOG_DEBUG(0, "Wrote \"%s\" (0x%08X bytes)", path, seg->memsz);
        }
    }

    LOG_DEBUG(0, "Ghidra import: ARM Cortex, little-endian, THUMB; set each segment's image base to the logged vaddr.");
    return 0;
}
