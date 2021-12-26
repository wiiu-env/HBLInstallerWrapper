#include <string.h>
#include <coreinit/title.h>
#include <sysapp/title.h>
#include <sysapp/launch.h>
#include <coreinit/ios.h>
#include <coreinit/cache.h>
#include "../homebrew_launcher_installer/src/elf_abi.h"
#include "payload_elf.h"

static unsigned int load_elf_image (const uint8_t* elfstart) {
    Elf32_Ehdr *ehdr;
    Elf32_Phdr *phdrs;
    unsigned char *image;
    int i;

    ehdr = (Elf32_Ehdr *) elfstart;

    if(ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
        return 0;

    if(ehdr->e_phentsize != sizeof(Elf32_Phdr))
        return 0;

    phdrs = (Elf32_Phdr*)(elfstart + ehdr->e_phoff);

    for(i = 0; i < ehdr->e_phnum; i++) {
        if(phdrs[i].p_type != PT_LOAD)
            continue;

        if(phdrs[i].p_filesz > phdrs[i].p_memsz)
            continue;

        if(!phdrs[i].p_filesz)
            continue;

        unsigned int p_paddr = phdrs[i].p_paddr;
        image = (unsigned char *) (elfstart + phdrs[i].p_offset);

        memcpy ((void *) p_paddr, image, phdrs[i].p_filesz);
        DCFlushRange((void*)p_paddr, phdrs[i].p_filesz);

        if(phdrs[i].p_flags & PF_X)
            ICInvalidateRange ((void *) p_paddr, phdrs[i].p_memsz);
    }

    //! clear BSS
    Elf32_Shdr *shdr = (Elf32_Shdr *) (elfstart + ehdr->e_shoff);
    for(i = 0; i < ehdr->e_shnum; i++) {
        const char *section_name = ((const char*)elfstart) + shdr[ehdr->e_shstrndx].sh_offset + shdr[i].sh_name;
        if(section_name[0] == '.' && section_name[1] == 'b' && section_name[2] == 's' && section_name[3] == 's') {
            memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
            DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
        } else if(section_name[0] == '.' && section_name[1] == 's' && section_name[2] == 'b' && section_name[3] == 's' && section_name[4] == 's') {
            memset((void*)shdr[i].sh_addr, 0, shdr[i].sh_size);
            DCFlushRange((void*)shdr[i].sh_addr, shdr[i].sh_size);
        }
    }

    return ehdr->e_entry;
}

int main(int argc, char **argv) {
    uint32_t res = load_elf_image(payload_elf);
    if(res != 0) {
        ((int (*)(int, char **)) res)(0, nullptr);
    }
    
    return 0;
}
