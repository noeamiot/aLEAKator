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

    // Prepare adresses
    ldr r0, =a0
    ldr r1, =a1
    ldr r2, =m
    ldr r3, =blk

    ldr r4, [r0]    // a0 in r4
    ldr r5, [r2]   // m in r5

    // Refresh a0
    eor r4, r4, r5
    str r4, [r0]

    str r3, [r3] // Clear write LSU path
    eor r4, r4, r4 // Clear r4
    mov r4, 0x0 // Clear alu path [suposedly, in fact not enough]

    // Last value read is m so read LSU path is already cleared from a0
    ldr r6, [r1]   // a1 in r12
    // Refresh a1
    eor r6, r6, r5
    str r6, [r2]
    mov r6, 0x0

    .rept 8
    eor r12, r12, r12
    .endr

    mov r0, #0
    pop {r4}
    bx lr
.L4:
    .align 2
.L3:
    .word .LANCHOR0
    .size main, .-main
    .global blk
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
    .type m, %object
    .size m, 4
m:
    .word   0x0000000F
    .type blk, %object
    .size blk, 4
blk:
    .space 4
