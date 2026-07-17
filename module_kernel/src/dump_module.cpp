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
 *
 * ksceKernelGetModuleInfo lives in SceModulemgrForKernel, which the runtime
 * kernel loader will NOT resolve for late-loaded plugins (you get
 * "Library not found: [SceModulemgrForKernel, ver=1]" at boot and the plugin
 * never starts). Instead, we resolve it at runtime through module_get_export_func
 * from taihenModuleUtils -- the same pattern used by FAGDec.
 *
 * CMakeLists changes required (vs the broken version):
 *   REMOVE: SceModulemgrForKernel_stub
 *   ADD:    taihenModuleUtils_stub
 ******************************************************************************/

#include "dump_module.h"

#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysclib.h>
#include <taihen.h>

#include "log.h"
#include "logfile.h"  // For LOGFILE_DIR_PARENT (ux0:<PROJECT_NAME>).

// module_get_export_func is exported by taihenModuleUtils but only declared in
// taiHEN's internal module.h -- public taihen.h omits it. Forward-declare with
// C linkage so the stub resolves at link time. Signature is verbatim from
// upstream taiHEN module.h.
extern "C" int module_get_export_func(SceUID pid, const char* modname, uint32_t libnid, uint32_t funcnid,
                                      uintptr_t* func);

// ksceKernelGetModuleInfo NID from db/360/SceKernelModulemgr.yml
// Library NID: SceModulemgrForKernel = 0xC445FA63
// Function NID: ksceKernelGetModuleInfo = 0xD269F915
#define NID_LIB_SceModulemgrForKernel 0xC445FA63
#define NID_FN_ksceKernelGetModuleInfo 0xD269F915

typedef int (*GetModuleInfo_t)(SceUID pid, SceUID modid, SceKernelModuleInfo* info);
static GetModuleInfo_t s_getModuleInfo = NULL;

#define DUMP_DIR LOGFILE_DIR_PARENT "/dumps/"
#define COPY_CHUNK 0x8000

static unsigned char g_bounce[COPY_CHUNK];

// Call once at module_start, after taiHEN is ready.
int dump_module_init(void) {
    int ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", NID_LIB_SceModulemgrForKernel,
                                     NID_FN_ksceKernelGetModuleInfo, (uintptr_t*)&s_getModuleInfo);
    if (ret < 0) {
        LOG_ERROR("module_get_export_func(ksceKernelGetModuleInfo) = 0x%08X", ret);
        s_getModuleInfo = NULL;
    } else {
        LOG_DEBUG(0, "ksceKernelGetModuleInfo resolved at 0x%08X", (unsigned)(uintptr_t)s_getModuleInfo);
    }
    return ret;
}

int dump_module(const char* module_name, unsigned int expected_nid) {
    if (s_getModuleInfo == NULL) {
        LOG_ERROR("dump_module: not initialised, call dump_module_init() first");
        return -1;
    }

    // Resolve the module to a modid and verify NID.
    tai_module_info_t tinfo;
    tinfo.size = sizeof(tinfo);
    int ret = taiGetModuleInfoForKernel(KERNEL_PID, module_name, &tinfo);
    if (ret < 0) {
        LOG_ERROR("taiGetModuleInfoForKernel(\"%s\") = 0x%08X", module_name, ret);
        return ret;
    }
    LOG_DEBUG(0, "Module \"%s\": modid=0x%08X nid=0x%08X", module_name, tinfo.modid, tinfo.module_nid);

    // Guard against reversing the wrong build's offsets.
    if (expected_nid != 0 && tinfo.module_nid != expected_nid) {
        LOG_WARN("Module NID 0x%08X != expected 0x%08X -- offsets from this dump would not match. Aborting.",
                 tinfo.module_nid, expected_nid);
        return -1;
    }

    // Fetch segment layout via the runtime-resolved function.
    SceKernelModuleInfo minfo;
    minfo.size = sizeof(minfo);
    ret = s_getModuleInfo(KERNEL_PID, tinfo.modid, &minfo);
    if (ret < 0) {
        LOG_ERROR("ksceKernelGetModuleInfo = 0x%08X", ret);
        return ret;
    }

    // Ensure output directory exists.
    ksceIoMkdir(LOGFILE_DIR_PARENT, 0777);
    ksceIoMkdir(DUMP_DIR, 0777);

    for (int i = 0; i < 4; i++) {
        SceKernelSegmentInfo* seg = &minfo.segments[i];
        if (seg->size == 0 || seg->memsz == 0 || seg->vaddr == NULL) {
            continue;
        }

        LOG_DEBUG(0, "seg[%d] vaddr=0x%08X memsz=0x%08X filesz=0x%08X perms=0x%X", i, (unsigned)(uintptr_t)seg->vaddr,
                  seg->memsz, seg->filesz, seg->perms);

        // Filename encodes load vaddr -- use it as Ghidra's image base for this segment.
        char path[256];
        int n =
            snprintf(path, sizeof(path), DUMP_DIR "%s_seg%d_0x%08X.bin", module_name, i, (unsigned)(uintptr_t)seg->vaddr);
        if (n < 0 || n >= (int)sizeof(path)) {
            LOG_ERROR("snprintf truncated path for seg %d", i);
            continue;
        }

        SceUID fd = ksceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
        if (fd < 0) {
            LOG_ERROR("ksceIoOpen(\"%s\") = 0x%08X", path, fd);
            continue;
        }

        const unsigned char* src = (const unsigned char*)seg->vaddr;
        SceSize remaining = seg->memsz;
        SceSize offset = 0;
        bool ok = true;
        while (remaining > 0) {
            SceSize chunk = remaining > COPY_CHUNK ? COPY_CHUNK : remaining;
            memcpy(g_bounce, src + offset, chunk);
            int w = ksceIoWrite(fd, g_bounce, chunk);
            if (w < 0) {
                LOG_ERROR("ksceIoWrite seg %d +0x%08X = 0x%08X", i, offset, w);
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

    LOG_DEBUG(0,
              "Ghidra: raw binary, ARM little-endian THUMB, set each segment's image base to the vaddr in its filename.");
    return 0;
}
