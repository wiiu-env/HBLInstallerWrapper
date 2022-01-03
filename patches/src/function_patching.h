/* based on blsug.h
 *   by Alex Chadwick
 *
 * Copyright (C) 2014, Alex Chadwick
 * Modified by Maschell, 2018
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <stdint.h>
#include <coreinit/dynload.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SECTION(x) __attribute__((__section__ (".function." x)))

typedef enum library_type_t {
    LIBRARY_AVM,
    LIBRARY_CAMERA,
    LIBRARY_COREINIT,
    LIBRARY_DC,
    LIBRARY_DMAE,
    LIBRARY_DRMAPP,
    LIBRARY_ERREULA,
    LIBRARY_GX2,
    LIBRARY_H264,
    LIBRARY_LZMA920,
    LIBRARY_MIC,
    LIBRARY_NFC,
    LIBRARY_NIO_PROF,
    LIBRARY_NLIBCURL,
    LIBRARY_NLIBNSS,
    LIBRARY_NLIBNSS2,
    LIBRARY_NN_AC,
    LIBRARY_NN_ACP,
    LIBRARY_NN_ACT,
    LIBRARY_NN_AOC,
    LIBRARY_NN_BOSS,
    LIBRARY_NN_CCR,
    LIBRARY_NN_CMPT,
    LIBRARY_NN_DLP,
    LIBRARY_NN_EC,
    LIBRARY_NN_FP,
    LIBRARY_NN_HAI,
    LIBRARY_NN_HPAD,
    LIBRARY_NN_IDBE,
    LIBRARY_NN_NDM,
    LIBRARY_NN_NETS2,
    LIBRARY_NN_NFP,
    LIBRARY_NN_NIM,
    LIBRARY_NN_OLV,
    LIBRARY_NN_PDM,
    LIBRARY_NN_SAVE,
    LIBRARY_NN_SL,
    LIBRARY_NN_SPM,
    LIBRARY_NN_TEMP,
    LIBRARY_NN_UDS,
    LIBRARY_NN_VCTL,
    LIBRARY_NSYSCCR,
    LIBRARY_NSYSHID,
    LIBRARY_NSYSKBD,
    LIBRARY_NSYSNET,
    LIBRARY_NSYSUHS,
    LIBRARY_NSYSUVD,
    LIBRARY_NTAG,
    LIBRARY_PADSCORE,
    LIBRARY_PROC_UI,
    LIBRARY_SND_CORE,
    LIBRARY_SND_USER,
    LIBRARY_SNDCORE2,
    LIBRARY_SNDUSER2,
    LIBRARY_SWKBD,
    LIBRARY_SYSAPP,
    LIBRARY_TCL,
    LIBRARY_TVE,
    LIBRARY_UAC,
    LIBRARY_UAC_RPL,
    LIBRARY_USB_MIC,
    LIBRARY_UVC,
    LIBRARY_UVD,
    LIBRARY_VPAD,
    LIBRARY_VPADBASE,
    LIBRARY_ZLIB125,
    LIBRARY_OTHER,
} library_type_t;

typedef enum function_patcher_entry_type_t {
    ENTRY_FUNCTION,
    ENTRY_FUNCTION_MANDATORY,
    ENTRY_EXPORT
} function_patcher_entry_type_t;

typedef enum WUPSFPTargetProcess {
    TARGET_PROCESS_ALL                   = 0xFF,
    TARGET_PROCESS_ROOT_RPX              = 1,
    TARGET_PROCESS_WII_U_MENU            = 2,
    TARGET_PROCESS_TVII                  = 3,
    TARGET_PROCESS_E_MANUAL              = 4,
    TARGET_PROCESS_HOME_MENU             = 5,
    TARGET_PROCESS_ERROR_DISPLAY         = 6,
    TARGET_PROCESS_MINI_MIIVERSE         = 7,   
    TARGET_PROCESS_BROWSER               = 8,
    TARGET_PROCESS_MIIVERSE              = 9,
    TARGET_PROCESS_ESHOP                 = 10,
    TARGET_PROCESS_PFID_11               = 11,
    TARGET_PROCESS_DOWNLOAD_MANAGER      = 12,
    TARGET_PROCESS_PFID_13               = 13,
    TARGET_PROCESS_PFID_14               = 14,
    TARGET_PROCESS_GAME                  = 15,
    TARGET_PROCESS_GAME_AND_MENU         = 16,
} TargetProcess;

#define FUNCTION_PATCHER_METHOD_STORE_SIZE 0x40

typedef struct function_patcher_entry_t {
    function_patcher_entry_type_t type;
    struct {
        const void *                        physical_address;   /* (optional) Physical Address. If set, the name and lib will be ignored */
        const void *                        virtual_address;    /* (optional) Physical Address. If set, the name and lib will be ignored */
        const char *                        name;               /* Name of the function that will be replaced */
        const library_type_t    library;            /**/
        const char *                        my_function_name;   /* Function name of your own, new function (my_XXX) */
        const void *                        target;             /* Address of our own, new function (my_XXX)*/
        const void *                        call_addr;          /* Address for calling the real function.(real_XXX) */
        volatile uint32_t                   replace_data [FUNCTION_PATCHER_METHOD_STORE_SIZE];  /* [will be filled] Space for us to store some jump instructions */
        const TargetProcess                  targetProcess;      /* Target process*/
        bool                                isAlreadyPatched;      /* Target process*/
    } _function;
} function_patcher_entry_t;

#define MUST_REPLACE_PHYSICAL(x, physical_address, virtual_address) MUST_REPLACE_PHYSICAL_FOR_PROCESS(x, physical_address, virtual_address, TARGET_PROCESS_GAME_AND_MENU)
#define MUST_REPLACE_PHYSICAL_FOR_PROCESS(x, physical_address, virtual_address, targetProcess) MUST_REPLACE_EX(physical_address, virtual_address, real_ ## x, LIBRARY_OTHER, my_ ## x, x, targetProcess)

#define MUST_REPLACE(x, lib, function_name) MUST_REPLACE_FOR_PROCESS(x, lib, function_name, TARGET_PROCESS_GAME_AND_MENU)
#define MUST_REPLACE_FOR_PROCESS(x, lib, function_name, targetProcess) MUST_REPLACE_EX(NULL, NULL, real_ ## x, lib, my_ ## x,  function_name, targetProcess)

#define MUST_REPLACE_EX(pAddress, vAddress, original_func, rpl_type, replace_func, replace_function_name, process) \
    extern const function_patcher_entry_t load_ ## replace_func \
        SECTION("hooks"); \
    const function_patcher_entry_t load_ ## replace_func = { \
        .type = ENTRY_FUNCTION_MANDATORY, \
        ._function = { \
                .physical_address = (const void*) pAddress, \
                .virtual_address = (const void*) vAddress, \
                .name = #replace_function_name, \
                .library = rpl_type, \
                .my_function_name = #replace_func, \
                .target = (const void*)&(replace_func), \
                .call_addr = (const void*)&(original_func),\
                .replace_data = {},   \
                .targetProcess =  process, \
                .isAlreadyPatched =  false, \
            } \
    }

#define DECL_FUNCTION(res, name, ...) \
        res (* real_ ## name)(__VA_ARGS__) __attribute__((section(".data"))); \
        res my_ ## name(__VA_ARGS__)

typedef struct rpl_handling {
    library_type_t library;
    const char rplname[15];
    OSDynLoad_Module handle;
} rpl_handling;

#ifdef __cplusplus
}
#endif