# This file has been modified from the ibex example with the following license:
# Copyright lowRISC contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

.section .text

reset_handler:
  /* set all registers to zero */
/*  mv  x1, x0 */
/*  mv  x2, x1 */
/*  mv  x3, x1 */
/*  mv  x4, x1 */
/*  mv  x5, x1 */
/*  mv  x6, x1 */
/*  mv  x7, x1 */
/*  mv  x8, x1 */
/*  mv  x9, x1 */
/*  mv x10, x1 */
/*  mv x11, x1 */
/*  mv x12, x1 */
/*  mv x13, x1 */
/*  mv x14, x1 */
/*  mv x15, x1 */
/*  mv x16, x1 */
/*  mv x17, x1 */
/*  mv x18, x1 */
/*  mv x19, x1 */
/*  mv x20, x1 */
/*  mv x21, x1 */
/*  mv x22, x1 */
/*  mv x23, x1 */
/*  mv x24, x1 */
/*  mv x25, x1 */
/*  mv x26, x1 */
/*  mv x27, x1 */
/*  mv x28, x1 */
/*  mv x29, x1 */
/*  mv x30, x1 */
/*  mv x31, x1 */

  /* stack initilization */
  la   x2, _sram_stacktop

/* Both these symbols must be visible globally */
.global _start
.global _hang

/* jump to main program entry point (argc = argv = 0) */
_start:
  jal x1, main

  /* If execution ends up here just put the core to sleep */
_hang:
  j _hang

/* =================================================== [ exceptions ] === */
/* This section has to be down here, since we have to disable rvc for it  */

  .section .vectors, "ax"
  .option norvc;

  /* All unimplemented interrupts/exceptions go to hang. */
  .org 0x00
  .rept 31
  jal x0, _hang
  .endr

  /* reset vector */
  .org 0x80
  jal x0, reset_handler
