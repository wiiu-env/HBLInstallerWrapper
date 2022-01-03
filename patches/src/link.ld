OUTPUT(patches.elf);

ENTRY(_start);

SECTIONS {
    . = 0x00801700;
    .text : {
        *(.text*);
    }
    .function.hooks : {
      *(.function.hooks*)
      KEEP(*(.function.hooks*))
    }
    .data : {
        *(.rodata*);
        *(.data*);
        *(.bss*);

    }
    /DISCARD/ : {
        *(*);
    }
}

ASSERT((SIZEOF(.text) + SIZEOF(.data)) < 0x900, "Memory overlapping with main elf.");