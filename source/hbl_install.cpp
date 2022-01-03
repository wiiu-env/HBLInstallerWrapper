#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <coreinit/dynload.h>
#include <coreinit/debug.h>
#include <coreinit/memorymap.h>
#include <coreinit/cache.h>
#include "../homebrew_launcher_installer/sd_loader/src/common.h"
#include "../homebrew_launcher_installer/sd_loader/src/elf_abi.h"
#include "sd_loader_elf.h"
#include "setup_syscalls.h"

#define address_LiWaitIopComplete                   0x01010180
#define address_LiWaitIopCompleteWithInterrupts     0x0101006C
#define address_LiWaitOneChunk                      0x0100080C
#define address_PrepareTitle_hook                   0xFFF184E4
#define address_sgIsLoadingBuffer                   0xEFE19E80
#define address_gDynloadInitialized                 0xEFE13DBC

#define ADDRESS_OSTitle_main_entry_ptr              0x1005E040
#define ADDRESS_main_entry_hook                     0x0101c56c

/* assembly functions */
extern "C" void Syscall_0x36(void);
extern "C" void KernelPatches(void);

void __attribute__ ((noinline)) kern_write(void *addr, uint32_t value);

static void InstallPatches();

static unsigned int load_elf_image(const uint8_t *elfstart) {
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdrs;
    unsigned char *image;
    int i;

    ehdr = (Elf32_Ehdr *) elfstart;

    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
        return 0;

    if (ehdr->e_phentsize != sizeof(Elf32_Phdr))
        return 0;

    phdrs = (Elf32_Phdr *) (elfstart + ehdr->e_phoff);

    for (i = 0; i < ehdr->e_phnum; i++) {
        if (phdrs[i].p_type != PT_LOAD)
            continue;

        if (phdrs[i].p_filesz > phdrs[i].p_memsz)
            continue;

        if (!phdrs[i].p_filesz)
            continue;

        unsigned int p_paddr = phdrs[i].p_paddr;
        image = (unsigned char *) (elfstart + phdrs[i].p_offset);

        memcpy((void *) p_paddr, image, phdrs[i].p_filesz);
        DCFlushRange((void *) p_paddr, phdrs[i].p_filesz);

        if (phdrs[i].p_flags & PF_X)
            ICInvalidateRange((void *) p_paddr, phdrs[i].p_memsz);
    }

    //! clear BSS
    Elf32_Shdr *shdr = (Elf32_Shdr *) (elfstart + ehdr->e_shoff);
    for (i = 0; i < ehdr->e_shnum; i++) {
        const char *section_name = ((const char *) elfstart) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
        if (section_name[0] == '.' && section_name[1] == 'b' && section_name[2] == 's' && section_name[3] == 's') {
            memset((void *) shdr[i].sh_addr, 0, shdr[i].sh_size);
            DCFlushRange((void *) shdr[i].sh_addr, shdr[i].sh_size);
        } else if (section_name[0] == '.' && section_name[1] == 's' && section_name[2] == 'b' && section_name[3] == 's' && section_name[4] == 's') {
            memset((void *) shdr[i].sh_addr, 0, shdr[i].sh_size);
            DCFlushRange((void *) shdr[i].sh_addr, shdr[i].sh_size);
        }
    }

    return ehdr->e_entry;
}

void KernelWriteU32(uint32_t addr, uint32_t value) {
    ICInvalidateRange(&value, 4);
    DCFlushRange(&value, 4);

    auto dst = (uint32_t) OSEffectiveToPhysical((uint32_t) addr);
    auto src = (uint32_t) OSEffectiveToPhysical((uint32_t) &value);

    SC_0x25_KernelCopyData(dst, src, 4);

    DCFlushRange((void *) addr, 4);
    ICInvalidateRange((void *) addr, 4);
}

void InstallHBL() {
    kern_write((void *) (KERN_SYSCALL_TBL_1 + (0x36 * 4)), (unsigned int) KernelPatches);
    kern_write((void *) (KERN_SYSCALL_TBL_2 + (0x36 * 4)), (unsigned int) KernelPatches);
    kern_write((void *) (KERN_SYSCALL_TBL_3 + (0x36 * 4)), (unsigned int) KernelPatches);
    kern_write((void *) (KERN_SYSCALL_TBL_4 + (0x36 * 4)), (unsigned int) KernelPatches);
    kern_write((void *) (KERN_SYSCALL_TBL_5 + (0x36 * 4)), (unsigned int) KernelPatches);

    Syscall_0x36();

    InstallPatches();

    auto *pElfBuffer = (unsigned char *) sd_loader_elf; // use this address as temporary to load the elf
    unsigned int mainEntryPoint = load_elf_image(pElfBuffer);

    if (mainEntryPoint == 0) {
        OSFatal("failed to load elf");
    }

    //! Install our entry point hook
    unsigned int repl_addr = ADDRESS_main_entry_hook;
    unsigned int jump_addr = mainEntryPoint & 0x03fffffc;

    unsigned int bufferU32 = 0x48000003 | jump_addr;
    KernelWriteU32(repl_addr, bufferU32);
}

/* ****************************************************************** */
/*                         INSTALL PATCHES                            */
/* All OS specific stuff is done here                                 */
/* ****************************************************************** */
static void InstallPatches() {
    OsSpecifics osSpecificFunctions;
    memset(&osSpecificFunctions, 0, sizeof(OsSpecifics));

    unsigned int bufferU32;
    /* Pre-setup a few options to defined values */
    bufferU32 = 550;
    memcpy((void *) &OS_FIRMWARE, &bufferU32, sizeof(bufferU32));
    bufferU32 = 0xDEADC0DE;
    memcpy((void *) &MAIN_ENTRY_ADDR, &bufferU32, sizeof(bufferU32));
    memcpy((void *) &ELF_DATA_ADDR, &bufferU32, sizeof(bufferU32));
    bufferU32 = 0;
    memcpy((void *) &ELF_DATA_SIZE, &bufferU32, sizeof(bufferU32));
    memcpy((void *) &HBL_CHANNEL, &bufferU32, sizeof(bufferU32));

    osSpecificFunctions.addr_OSDynLoad_Acquire = (unsigned int) OSDynLoad_Acquire;
    osSpecificFunctions.addr_OSDynLoad_FindExport = (unsigned int) OSDynLoad_FindExport;

    osSpecificFunctions.addr_KernSyscallTbl1 = KERN_SYSCALL_TBL_1;
    osSpecificFunctions.addr_KernSyscallTbl2 = KERN_SYSCALL_TBL_2;
    osSpecificFunctions.addr_KernSyscallTbl3 = KERN_SYSCALL_TBL_3;
    osSpecificFunctions.addr_KernSyscallTbl4 = KERN_SYSCALL_TBL_4;
    osSpecificFunctions.addr_KernSyscallTbl5 = KERN_SYSCALL_TBL_5;

    osSpecificFunctions.LiWaitIopComplete = (int (*)(int, int *)) address_LiWaitIopComplete;
    osSpecificFunctions.LiWaitIopCompleteWithInterrupts = (int (*)(int, int *)) address_LiWaitIopCompleteWithInterrupts;
    osSpecificFunctions.addr_LiWaitOneChunk = address_LiWaitOneChunk;
    osSpecificFunctions.addr_PrepareTitle_hook = address_PrepareTitle_hook;
    osSpecificFunctions.addr_sgIsLoadingBuffer = address_sgIsLoadingBuffer;
    osSpecificFunctions.addr_gDynloadInitialized = address_gDynloadInitialized;
    osSpecificFunctions.orig_LiWaitOneChunkInstr = *(unsigned int *) address_LiWaitOneChunk;

    //! pointer to main entry point of a title
    osSpecificFunctions.addr_OSTitle_main_entry = ADDRESS_OSTitle_main_entry_ptr;

    memcpy((void *) OS_SPECIFICS, &osSpecificFunctions, sizeof(OsSpecifics));
}
