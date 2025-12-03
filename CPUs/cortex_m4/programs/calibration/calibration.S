    .cpu cortex-m3
    .arch armv7-m
    .fpu softvfp
    .eabi_attribute 20, 1
    .eabi_attribute 21, 1
    .eabi_attribute 23, 3
    .eabi_attribute 24, 1
    .eabi_attribute 25, 1
    .eabi_attribute 26, 1
    .eabi_attribute 30, 1
    .eabi_attribute 34, 1
    .eabi_attribute 18, 4
    .text
    .align 1
    .global main
    .syntax unified
    .thumb
    .thumb_func
    .type main, %function
main:
    @ args = 0, pretend = 0, frame = 0
    @ frame_needed = 0, uses_anonymous_args = 0
    @ link register save eliminated.
    ldr r0, =a0
    ldr r1, [r0] // a0 in r1

    ldr r0, =res
    ldr r3, [r0] // Blank load in r3 to prevent memory read transition

    ldr r0, =a1
    ldr r2, [r0] // a1 in r2

    eor r3, r2, r1 // a0 xor a1 in r3

    // NOPs
    eor r4, r0, r0
    eor r4, r0, r0
    eor r4, r0, r0
    eor r4, r0, r0
    eor r4, r0, r0
    eor r4, r0, r0

    ldr r4, =res
    str r3, [r4] // store r3 in res
    bx lr
.L4:
    .align 2
.L3:
    .word .LANCHOR0
    .size main, .-main
    .global res
    .global a1
    .global a0
    .data
    .align 2
    .set .LANCHOR0,. + 0
    .type a0, %object
    .size a0, 4
a0:
    .word   0xFAFFFFFF
    .type a1, %object
    .size a1, 4
a1:
    .word   0x0000000F
    .type res, %object
    .size res, 4
res:
    .space 4
