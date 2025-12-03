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
