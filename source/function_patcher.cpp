#include <cstring>
#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/memorymap.h>
#include <coreinit/debug.h>
#include "utils/logger.h"
#include "../patches/src/function_patching.h"
#include "elfio/elfio.hpp"
#include "setup_syscalls.h"

using namespace ELFIO;

bool isDynamicFunction(uint32_t physicalAddress) {
    if ((physicalAddress & 0x80000000) == 0x80000000) {
        return true;
    }
    return false;
}

rpl_handling rpl_handles[] __attribute__((section(".data"))) = {
        {LIBRARY_AVM,      "avm.rpl",      nullptr},
        {LIBRARY_CAMERA,   "camera.rpl",   nullptr},
        {LIBRARY_COREINIT, "coreinit.rpl", nullptr},
        {LIBRARY_DC,       "dc.rpl",       nullptr},
        {LIBRARY_DMAE,     "dmae.rpl",     nullptr},
        {LIBRARY_DRMAPP,   "drmapp.rpl",   nullptr},
        {LIBRARY_ERREULA,  "erreula.rpl",  nullptr},
        {LIBRARY_GX2,      "gx2.rpl",      nullptr},
        {LIBRARY_H264,     "h264.rpl",     nullptr},
        {LIBRARY_LZMA920,  "lzma920.rpl",  nullptr},
        {LIBRARY_MIC,      "mic.rpl",      nullptr},
        {LIBRARY_NFC,      "nfc.rpl",      nullptr},
        {LIBRARY_NIO_PROF, "nio_prof.rpl", nullptr},
        {LIBRARY_NLIBCURL, "nlibcurl.rpl", nullptr},
        {LIBRARY_NLIBNSS,  "nlibnss.rpl",  nullptr},
        {LIBRARY_NLIBNSS2, "nlibnss2.rpl", nullptr},
        {LIBRARY_NN_AC,    "nn_ac.rpl",    nullptr},
        {LIBRARY_NN_ACP,   "nn_acp.rpl",   nullptr},
        {LIBRARY_NN_ACT,   "nn_act.rpl",   nullptr},
        {LIBRARY_NN_AOC,   "nn_aoc.rpl",   nullptr},
        {LIBRARY_NN_BOSS,  "nn_boss.rpl",  nullptr},
        {LIBRARY_NN_CCR,   "nn_ccr.rpl",   nullptr},
        {LIBRARY_NN_CMPT,  "nn_cmpt.rpl",  nullptr},
        {LIBRARY_NN_DLP,   "nn_dlp.rpl",   nullptr},
        {LIBRARY_NN_EC,    "nn_ec.rpl",    nullptr},
        {LIBRARY_NN_FP,    "nn_fp.rpl",    nullptr},
        {LIBRARY_NN_HAI,   "nn_hai.rpl",   nullptr},
        {LIBRARY_NN_HPAD,  "nn_hpad.rpl",  nullptr},
        {LIBRARY_NN_IDBE,  "nn_idbe.rpl",  nullptr},
        {LIBRARY_NN_NDM,   "nn_ndm.rpl",   nullptr},
        {LIBRARY_NN_NETS2, "nn_nets2.rpl", nullptr},
        {LIBRARY_NN_NFP,   "nn_nfp.rpl",   nullptr},
        {LIBRARY_NN_NIM,   "nn_nim.rpl",   nullptr},
        {LIBRARY_NN_OLV,   "nn_olv.rpl",   nullptr},
        {LIBRARY_NN_PDM,   "nn_pdm.rpl",   nullptr},
        {LIBRARY_NN_SAVE,  "nn_save.rpl",  nullptr},
        {LIBRARY_NN_SL,    "nn_sl.rpl",    nullptr},
        {LIBRARY_NN_SPM,   "nn_spm.rpl",   nullptr},
        {LIBRARY_NN_TEMP,  "nn_temp.rpl",  nullptr},
        {LIBRARY_NN_UDS,   "nn_uds.rpl",   nullptr},
        {LIBRARY_NN_VCTL,  "nn_vctl.rpl",  nullptr},
        {LIBRARY_NSYSCCR,  "nsysccr.rpl",  nullptr},
        {LIBRARY_NSYSHID,  "nsyshid.rpl",  nullptr},
        {LIBRARY_NSYSKBD,  "nsyskbd.rpl",  nullptr},
        {LIBRARY_NSYSNET,  "nsysnet.rpl",  nullptr},
        {LIBRARY_NSYSUHS,  "nsysuhs.rpl",  nullptr},
        {LIBRARY_NSYSUVD,  "nsysuvd.rpl",  nullptr},
        {LIBRARY_NTAG,     "ntag.rpl",     nullptr},
        {LIBRARY_PADSCORE, "padscore.rpl", nullptr},
        {LIBRARY_PROC_UI,  "proc_ui.rpl",  nullptr},
        {LIBRARY_SNDCORE2, "sndcore2.rpl", nullptr},
        {LIBRARY_SNDUSER2, "snduser2.rpl", nullptr},
        {LIBRARY_SND_CORE, "snd_core.rpl", nullptr},
        {LIBRARY_SND_USER, "snd_user.rpl", nullptr},
        {LIBRARY_SWKBD,    "swkbd.rpl",    nullptr},
        {LIBRARY_SYSAPP,   "sysapp.rpl",   nullptr},
        {LIBRARY_TCL,      "tcl.rpl",      nullptr},
        {LIBRARY_TVE,      "tve.rpl",      nullptr},
        {LIBRARY_UAC,      "uac.rpl",      nullptr},
        {LIBRARY_UAC_RPL,  "uac_rpl.rpl",  nullptr},
        {LIBRARY_USB_MIC,  "usb_mic.rpl",  nullptr},
        {LIBRARY_UVC,      "uvc.rpl",      nullptr},
        {LIBRARY_UVD,      "uvd.rpl",      nullptr},
        {LIBRARY_VPAD,     "vpad.rpl",     nullptr},
        {LIBRARY_VPADBASE, "vpadbase.rpl", nullptr},
        {LIBRARY_ZLIB125,  "zlib125.rpl",  nullptr}
};

uint32_t getAddressOfFunction(const char *functionName, library_type_t library) {
    uint32_t real_addr = 0;

    OSDynLoad_Module rpl_handle = nullptr;

    OSDynLoad_Error err = OS_DYNLOAD_OK;

    int32_t rpl_handles_size = sizeof rpl_handles / sizeof rpl_handles[0];

    for (int32_t i = 0; i < rpl_handles_size; i++) {
        if (rpl_handles[i].library == library) {
            if (rpl_handles[i].handle == nullptr) {
                //DEBUG_FUNCTION_LINE("Lets acquire handle for rpl: %s", rpl_handles[i].rplname);
                err = OSDynLoad_IsModuleLoaded((char *) rpl_handles[i].rplname, &rpl_handles[i].handle);
            }
            if (err != OS_DYNLOAD_OK || !rpl_handles[i].handle) {
                WHBLogWritef("%s failed to acquire %d %08X\n", rpl_handles[i].rplname, err, rpl_handles[i].handle);
                return 0;
            }
            rpl_handle = rpl_handles[i].handle;
            break;
        }
    }

    if (!rpl_handle) {
        DEBUG_FUNCTION_LINE("Failed to find the RPL handle for %s", functionName);
        return 0;
    }

    OSDynLoad_FindExport(rpl_handle, 0, functionName, reinterpret_cast<void **>(&real_addr));

    if (!real_addr) {
        DEBUG_FUNCTION_LINE("OSDynLoad_FindExport failed for %s", functionName);
        return 0;
    }

    if ((library == LIBRARY_NN_ACP) && (uint32_t) (*(volatile uint32_t *) (real_addr) & 0x48000002) == 0x48000000) {
        auto address_diff = (uint32_t) (*(volatile uint32_t *) (real_addr) & 0x03FFFFFC);
        if ((address_diff & 0x03000000) == 0x03000000) {
            address_diff |= 0xFC000000;
        }
        real_addr += (int32_t) address_diff;
        if ((uint32_t) (*(volatile uint32_t *) (real_addr) & 0x48000002) == 0x48000000) {
            return 0;
        }
    }

    return real_addr;
}

void PatchFunction(function_patcher_entry_t *function_data) {
    if (function_data->_function.isAlreadyPatched) {
        DEBUG_FUNCTION_LINE("%s is already patch, we can skip it", function_data->_function.name);
        return;
    }
    /* Patch branches to it.  */
    volatile uint32_t *space = function_data->_function.replace_data;

    DEBUG_FUNCTION_LINE_VERBOSE("Patching %s ...", function_data->_function.name);

    auto physical = (uint32_t) function_data->_function.physical_address;
    auto repl_addr = (uint32_t) function_data->_function.target;
    auto call_addr = (uint32_t) function_data->_function.call_addr;

    auto real_addr = (uint32_t) function_data->_function.virtual_address;
    if (function_data->_function.library != LIBRARY_OTHER) {
        real_addr = getAddressOfFunction(function_data->_function.name, function_data->_function.library);
    }

    if (!real_addr) {
        WHBLogWritef("");
        DEBUG_FUNCTION_LINE("OSDynLoad_FindExport failed for %s", function_data->_function.name);
        return;
    }

    DEBUG_FUNCTION_LINE_VERBOSE("%s is located at %08X!", function_data->_function.name, real_addr);

    if (function_data->_function.library != LIBRARY_OTHER) {
        physical = (uint32_t) OSEffectiveToPhysical(real_addr);
    }

    if (!physical) {
        WHBLogWritef("Error. Something is wrong with the physical address");
        return;
    }

    DEBUG_FUNCTION_LINE_VERBOSE("%s physical is located at %08X!", function_data->_function.name, physical);

    *(volatile uint32_t *) (call_addr) = (uint32_t) (space);

    auto targetAddr = (uint32_t) space;
    if (targetAddr < 0x00800000 || targetAddr >= 0x01000000) {
        targetAddr = (uint32_t) OSEffectiveToPhysical(targetAddr);
    } else {
        targetAddr = targetAddr + 0x30800000 - 0x00800000;
    }

    SC_0x25_KernelCopyData(targetAddr, physical, 4);

    ICInvalidateRange((void *) (space), 4);
    DCFlushRange((void *) (space), 4);

    space++;

    uint32_t restoreInstruction = space[-1];

    /*
    00808cfc 3d601234      lis        r11 ,0x1234
    00808d00 616b5678      ori        r11 ,r11 ,0x5678
    00808d04 7d6903a6      mtspr      CTR ,r11
    00808d08 4e800420      bctr
     */

    *space = 0x3d600000 | (((real_addr + 4) >> 16) & 0x0000FFFF);
    space++;   // lis        r11 ,0x1234
    *space = 0x616b0000 | ((real_addr + 4) & 0x0000ffff);
    space++;           // ori        r11 ,r11 ,0x5678
    *space = 0x7d6903a6;
    space++;                                            // mtspr      CTR ,r11
    *space = 0x4e800420;
    space++;                                            // bctr

    //setting jump back
    uint32_t replace_instr = 0x48000002 | (repl_addr & 0x03FFFFFC);

    // If the jump is too big or we want only patch for certain processes we need a trampoline
    if (repl_addr > 0x03FFFFFC || function_data->_function.targetProcess != TARGET_PROCESS_ALL) {
        auto repl_addr_test = (uint32_t) space;
        if (function_data->_function.targetProcess != TARGET_PROCESS_ALL) {
            // Only use patched function if OSGetUPID matches function_data->targetProcess
            *space = 0x3d600000 | (((uint32_t *) OSGetUPID)[0] & 0x0000FFFF);
            space++;               // lis        r11 ,0x0
            *space = 0x816b0000 | (((uint32_t *) OSGetUPID)[1] & 0x0000FFFF);
            space++;               // lwz        r11 ,0x0(r11)
            if (function_data->_function.targetProcess == TARGET_PROCESS_GAME_AND_MENU) {
                *space = 0x2c0b0000 | TARGET_PROCESS_WII_U_MENU;
                space++;                        // cmpwi      r11 ,WUPS_FP_TARGET_PROCESS_WII_U_MENU
                *space = 0x41820000 | 0x00000020;
                space++;                                          // beq        myfunc
                *space = 0x2c0b0000 | TARGET_PROCESS_GAME;
                space++;                              // cmpwi      r11 ,FP_TARGET_PROCESS_GAME
                *space = 0x41820000 | 0x00000018;
                space++;                                          // beq        myfunc
            } else {
                *space = 0x2c0b0000 | function_data->_function.targetProcess;
                space++;                        // cmpwi      r11 ,function_data->targetProcess
                *space = 0x41820000 | 0x00000018;
                space++;                                          // beq        myfunc
            }
            *space = 0x3d600000 | (((real_addr + 4) >> 16) & 0x0000FFFF);
            space++;                  // lis        r11 ,(real_addr + 4)@hi
            *space = 0x616b0000 | ((real_addr + 4) & 0x0000ffff);
            space++;                          // ori        r11 ,(real_addr + 4)@lo
            *space = 0x7d6903a6;
            space++;                                                           // mtspr      CTR ,r11
            *space = restoreInstruction;
            space++;                                    //
            *space = 0x4e800420;
            space++;                                                           // bctr
        }
// myfunc:
        *space = 0x3d600000 | (((repl_addr) >> 16) & 0x0000FFFF);
        space++;                          // lis        r11 ,repl_addr@hi
        *space = 0x616b0000 | ((repl_addr) & 0x0000ffff);
        space++;                                  // ori        r11 ,r11 ,repl_addr@lo
        *space = 0x7d6903a6;
        space++;                                                               // mtspr      CTR ,r11
        *space = 0x4e800420;
        space++;                                                               // bctr

        // Make sure the trampoline itself is usable.
        if ((repl_addr_test & 0x03FFFFFC) != repl_addr_test) {
            DEBUG_FUNCTION_LINE("%08X != %08X", (repl_addr_test & 0x03FFFFFC), repl_addr_test);
            OSFatal("Jump is impossible");
        }

        replace_instr = 0x48000002 | (repl_addr_test & 0x03FFFFFC);
    }

    if (space > &function_data->_function.replace_data[FUNCTION_PATCHER_METHOD_STORE_SIZE]) {
        OSFatal("The replacement data is too long.");
    }

    DEBUG_FUNCTION_LINE_VERBOSE("Write instruction %08X to %08X [%08X] on core %d", replace_instr, real_addr, physical, OSGetThreadAffinity(OSGetCurrentThread()) / 2);

    uint32_t replace_instruction = replace_instr;
    uint32_t physical_address = physical;
    uint32_t effective_address = real_addr;
    DCFlushRange(&replace_instruction, 4);
    DCFlushRange(&physical_address, 4);

    auto replace_instruction_physical = (uint32_t) &replace_instruction;

    if (replace_instruction_physical < 0x00800000 || replace_instruction_physical >= 0x01000000) {
        replace_instruction_physical = OSEffectiveToPhysical(replace_instruction_physical);
    } else {
        replace_instruction_physical = replace_instruction_physical + 0x30800000 - 0x00800000;
    }

    SC_0x25_KernelCopyData(physical_address, replace_instruction_physical, 4);
    ICInvalidateRange((void *) (effective_address), 4);

    if (!isDynamicFunction(physical)) {
        DEBUG_FUNCTION_LINE("Set isAlreadyPatched = true");
        function_data->_function.isAlreadyPatched = true;
    }
}
