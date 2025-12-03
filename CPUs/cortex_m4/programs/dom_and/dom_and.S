    .cpu cortex-m4
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
    push {r4}
    nop
    ldr r0, =m
    ldr r1, [r0] // m in r1
    ldr r0, =a0
    ldr r2, [r0] // a0 in r2
    ldr r0, =b1
    ldr r3, [r0] // b1 in r3
    and r4, r2, r3
    eor r4, r4, r1 // a0 and b1 xor m
    and r2, r0, r0 // vwg clear of a0 share
    and r1, r0, r0 // vwg clear of m

    ldr r0, =a1 // Moved for vwg
    ldr r12, [r0]

    and r10, r12, r3 // a1b1

    eor r9, r10, r4 // a1b1 xor a0b1 xor m
    ldr r0, =res0
    str r9, [r0]
    mov r9, #0 // Transition without glitches
    mov r10, #0 // Transition without glitches
    mov r4, #0 // vwg

    and r3, r0, r0 // Clear b1 share from register

    ldr r0, =b0
    ldr r6, [r0]
    ldr r0, =m
    ldr r1, [r0] // m in r1

    and r7, r12, r6
    eor r8, r7, r1 // a1 and b0 xor m in r8
    and r12, r0, r0 // vwg Clear a1 before reloading a0
    and r1, r0, r0 // vwg Clear m
    and r7, r0, r0 // vwg Clear unxored res

    ldr r0, =blk
    ldr r0, [r0] // m is still leaking as it is the last loaded element, clear lsu


    ldr r0, =a0
    ldr r2, [r0] // a0 in r2

    and r9, r2, r6
    eor r9, r9, r8
    ldr r10, =res1
    str r9, [r10]

    mov r0, #0
    pop {r4}
    bx lr
.L4:
    .align 2
.L3:
    .word .LANCHOR0
    .size main, .-main
    .global blk
    .global res1
    .global res0
    .global m
    .global b1
    .global b0
    .global a1
    .global a0
    .data
    .align 2
    .set .LANCHOR0,. + 0
    .type m, %object
    .size m, 4
m:
    .word 0x01234567
    .type a0, %object
    .size a0, 4
a0:
    .word   0xFAFFFFFF
    .type a1, %object
    .size a1, 4
a1:
    .word   0x0000000F
    .type b0, %object
    .size b0, 4
b0:
    .word   0xF0000000
    .type b1, %object
    .size b1, 4
b1:
    .word   0xF5FFFFFF
    .type res0, %object
    .size res0, 4
res0:
    .space 4
    .type res1, %object
    .size res1, 4
res1:
    .space 4
    .type tmp1, %object
    .size tmp1, 4
tmp1:
    .space 4
    .type blk, %object
    .size blk, 4
blk:
    .space 4
    .ident "GCC: (Arch Repository) 14.2.0"
