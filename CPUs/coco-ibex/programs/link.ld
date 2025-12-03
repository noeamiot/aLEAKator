OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)

/* Ram and rom from coco-ibex are different memories but they are addressed using
* the same starting space. As data is always read from memory, we can't make it boot like
* the others cpu as we can't have the prolog copy data from rom */
MEMORY {
  /* ROM (rwx) : ORIGIN = 0x000000, LENGTH = 1024 */
  ROM (rwx) : ORIGIN = 0x000000, LENGTH = 16384
  /*RAM (rwx) : ORIGIN = 0x000000, LENGTH = 2048*/
  /*RAM (rwx) : ORIGIN = 0x000000, LENGTH = 4096*/
  RAM (rwx) : ORIGIN = 0x000000, LENGTH = 512
}

SECTIONS
{
    . = 0x0;
    .text : {
        KEEP(*(.vectors))
        *(.text)
        *(.rodata)
        . = ALIGN(4);
    } > ROM

    .data : {
        . = ALIGN(4);
        _sram_sdata = .;
        *(.data);
        *(.sdata);
        . = ALIGN(4);
        _sram_edata = .;
    } > RAM AT > ROM

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
