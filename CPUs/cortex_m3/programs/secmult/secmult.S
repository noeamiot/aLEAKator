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
    .global gmul
    .syntax unified
    .thumb
    .thumb_func
    .type gmul, %function
gmul:
    @ args = 0, pretend = 0, frame = 0
    @ frame_needed = 0, uses_anonymous_args = 0
    @ link register save eliminated.
    mov     r4, #0
    mov r3, r0
    mov ip, #8
    mov r0, #0
.L2:
    #sbfx r2, r1, #0, #1
    lsl r2, r1, #31
    asr r2, r2, #31
    and r2, r2, 0xFF

    and r2, r2, r3
    eor r0, r0, r2
    sxtb r2, r3
    #sbfx r3, r3, #7, #1
    lsl r3, r3, #24
    asr r3, r3, #31
    and r3, r3, 0xFF


    and r3, r3, #27
    eor r3, r3, r2, lsl #1
    uxtb r3, r3
    lsr r1, r1, #1
    subs ip, ip, #1
    bne .L2
    bx lr
    .size gmul, .-gmul
    .align 1
    .global main
    .syntax unified
    .thumb
    .thumb_func
    .type main, %function
main:
    @ args = 0, pretend = 0, frame = 0
    @ frame_needed = 0, uses_anonymous_args = 0
    push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    ldr     r11, =a0
    ldrb r5, [r11]	@ zero_extendqisi2
    ldr     r11, =b0
    ldrb r6, [r11]	@ zero_extendqisi2
    ldr     r11, =m01
    ldrb r7, [r11]	@ zero_extendqisi2


    mov r1, r6
    mov r0, r5
    bl gmul // b0 gmul a0
    eor r0, r0, r7
    ldr r11, =c0
    str r0, [r11]

    // twog and others
    mov r0, #0
    // vwg
    mov r6, #0

    // twg
    ldr  r9, =blk
    ldr  r9, [r9]

    ldr  r9, =b1
    ldrb r9, [r9]	@ zero_extendqisi2

    mov r1, r9
    mov r0, r5
    bl gmul // a0 gmul b1
    eor r5, r7, r0

    // twog
    mov r0, #0
    // vwg
    mov r2, #0
    mov r3, #0
    eor r7, r0, r0


    ldr  r11, =a1
    ldrb r8, [r11]	@ zero_extendqisi2


    mov r1, r9
    mov r0, r8
    bl gmul // b1 gmul a1
    eor r9, r5, r0

    // vwg
    mov r0, #0
    mov r5, #0
    mov r2, #0
    mov r3, #0
    mov r1, #0

    // twg
    ldr  r1, =blk
    str  r1, [r1]

    ldr  r11, =b0
    ldrb r1, [r11]	@ zero_extendqisi2
    mov r0, r8
    bl gmul // b0 gmul a1
    eor r0, r9, r0
    ldr r11, =c1
    str r0, [r11]
    pop {r4, r5, r6, r7, r8, r9, r10, r11, pc}
.L7:
    .align 2
.L6:
    .word .LANCHOR0
    .size main, .-main
    .global m01
    .global b0
    .global a0
    .global blk
    .global c1
    .global c0
    .global b1
    .global a1
    .data
    .align 2
    .set .LANCHOR0,. + 0
    .type a0, %object
    .size a0, 4
a0:
    .word 20
    .type b1, %object
    .size b1, 4
b1:
    .word 11
    .type m01, %object
    .size m01, 4
m01:
    .word 154
    .type a1, %object
    .size a1, 4
a1:
    .word 25
    .type b0, %object
    .size b0, 4
b0:
    .word 230
    .type blk, %object
    .size blk, 4
blk:
    .space 4
    .type c0, %object
    .size c0, 4
c0:
    .space 4
    .type c1, %object
    .size c1, 4
c1:
    .space 4
