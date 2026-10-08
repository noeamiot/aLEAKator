OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)

/* Default ibex simple system memory is way bigger but the default mask for ram is 1MB
   If it is a problem later, we can increase cfg_device_addr_mask easily */
MEMORY {
  RAM (rwx) : ORIGIN = 0x00100000, LENGTH = 1M
}

SECTIONS
{
    . = 0x0;
    .text : {
        _text_start = .;
        KEEP(*(.vectors))
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
        _text_end = .;
    } > RAM

    /* Add a little space between sections */
    .data : ALIGN(16) {
        _data_start = .;
        . = ALIGN(4);
        *(.data);
        *(.data.*);
        *(.sdata);
        *(.sdata.*);
        . = ALIGN(4);
        _data_end = .;
    } > RAM

    _flash_sdata = LOADADDR(.data);

    .bss : {
        . = ALIGN(4);
        _bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
        . = ALIGN(4);
        _bss_end = .;
    } > RAM

    _sram_stacktop = ORIGIN(RAM) + LENGTH(RAM);
}
