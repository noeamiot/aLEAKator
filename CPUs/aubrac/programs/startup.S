.section .text

/* Both these symbols must be visible globally */
.global _start
.global _hang

  /* jump to main program entry point (argc = argv = 0) */
_start:
  jal x1, main
  j _hang

// Move appart the symbols so that they re not wrongly detected during jumps
.space 8
  /* If execution ends up here just put the core to sleep */
_hang:
  j _hang

  .section .vectors, "ax"
  .option norvc;

  la sp, _sram_stacktop
  jal _start
  j _hang
