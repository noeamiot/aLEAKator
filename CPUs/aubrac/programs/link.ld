OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)

MEMORY {
  /* Length is in bytes of memory */
  RAM (rwx) : ORIGIN = 0x04000000, LENGTH = 16384
}

SECTIONS
{
    . = 0x0;
    .text : {
        KEEP(*(.vectors))
        *(.text)
        *(.rodata)
        . = ALIGN(4);
    } > RAM

    .data : {
        . = ALIGN(4);
        _sram_sdata = .;
        *(.data);
        *(.sdata);
        . = ALIGN(4);
        _sram_edata = .;
    } > RAM

    _flash_sdata = LOADADDR(.data);

    .bss : {
        . = ALIGN(4);
        _sram_sbss = .;
        *(.bss)
        *(.sbss)
        *(COMMON)
        . = ALIGN(4);
        _sram_ebss = .;
    } > RAM

    _sram_stacktop = ORIGIN(RAM) + LENGTH(RAM);
}
