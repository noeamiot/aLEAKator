MEMORY {
  ROM (rx) : ORIGIN = 0x00000000, LENGTH = 8M
  RAM (rw) : ORIGIN = 0x20000000, LENGTH = 4M
}

SECTIONS
{
    . = 0x0;
    .text : {
        KEEP(*(.vectors))
        *(.text)
        *(.text.*)
        *(.rodata)
        . = ALIGN(4);
    } > ROM

    .data : {
        . = ALIGN(4);
        _sram_sdata = .;
        *(.data);
        *(.data.*)
        . = ALIGN(4);
        _sram_edata = .;
    } > RAM AT > ROM

    _flash_sdata = LOADADDR(.data);

    .bss : {
        . = ALIGN(4);
        _sram_sbss = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        _sram_ebss = .;
    } > RAM

    _sram_stacktop = ORIGIN(RAM) + LENGTH(RAM);
}
