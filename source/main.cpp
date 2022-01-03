#include <whb/log_module.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <coreinit/cache.h>
#include "../patches/src/function_patching.h"
#include "hbl_install.h"
#include "function_patcher.h"
#include "setup_syscalls.h"
#include "elfio/elfio.hpp"
#include "utils/logger.h"
#include <patches_elf.h>

void PatchFunctionsFromElf() {
    ELFIO::elfio reader;

    // Load ELF data
    if (!reader.load((char *) patches_elf, patches_elf_size)) {
        DEBUG_FUNCTION_LINE("Failed to copy patches");
        return;
    }
    uint32_t sec_num = reader.sections.size();
    for (uint32_t i = 0; i < sec_num; ++i) {
        ELFIO::section *psec = reader.sections[i];
        if (psec->get_name() == ".text" || psec->get_name() == ".data" || psec->get_name() == ".function.hooks") {
            auto addr = (uint32_t) psec->get_address();
            auto size = (uint32_t) psec->get_size();
            DEBUG_FUNCTION_LINE("Copy %s: %08X to %08X size %08X", psec->get_name().c_str(), psec->get_data(), addr, size);
            memcpy((void *) addr, psec->get_data(), size);
            ICInvalidateRange((void *) addr, size);
            DCFlushRange((void *) addr, size);
        }
    }
    for (uint32_t i = 0; i < sec_num; ++i) {
        ELFIO::section *psec = reader.sections[i];
        if (psec->get_name() == ".function.hooks") {
            auto addr = (uint32_t) psec->get_address();
            auto size = (uint32_t) psec->get_size();
            size_t entries_count = size / sizeof(function_patcher_entry_t);
            auto *entries = (function_patcher_entry_t *) addr;
            DEBUG_FUNCTION_LINE("function patches count %d", entries_count);
            if (entries != nullptr) {
                for (size_t j = 0; j < entries_count; j++) {
                    function_patcher_entry_t *cur_function = &entries[j];
                    PatchFunction(cur_function);
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }
    SetupSyscall();
    InstallHBL();
    PatchFunctionsFromElf();

    return 0;
}
