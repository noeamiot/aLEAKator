MEMORY {
  ROM (rx) : ORIGIN = 0x00000000, LENGTH = 8M
  RAM (rw) : ORIGIN = 0x20000000, LENGTH = 4M
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
    } > ROM

    /* Add a little space between sections */
    .data : ALIGN(16) {
        _data_start = .;
        *(.data)
        *(.data.*)
        *(.sdata);
        *(.sdata.*);
        *(.got)
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
