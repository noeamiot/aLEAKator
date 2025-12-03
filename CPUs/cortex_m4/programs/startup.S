.syntax unified
.thumb

.section .vectors, "x"
.word _sram_stacktop     /* 00: Reset value of the Main Stack Pointer */
.word _startup           /* 01: Reset value of the Program Counter (Reset) */
.word _hang              /* 02: Non-Maskable Interrupt */
.word _hang              /* 03: Hard Fault Interrupt */
.word _hang              /* 04: Memory Mgmt Interrupt */
.word _hang              /* 05: Bus Fault Interrupt */
.word _hang              /* 06: Usage Fault Interrupt */
.word _hang              /* 07: Reserved */
.word _hang              /* 08: Reserved */
.word _hang              /* 09: Reserved */
.word _hang              /* 10: Reserved */
.word _hang              /* 11: SVCall Interrupt */
.word _hang              /* 12: Debug Monitor Interrupt */
.word _hang              /* 13: Reserved */
.word _hang              /* 14: PendSV Interrupt */
.word _hang              /* 15: SysTick Interrupt */

/* Both these symbols must be visible globally */
.global _start
.global _hang

.text
.align
.thumb_func
_startup:
    /* zero all registers */
    mov r0, #0
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    mov r9, #0
    mov r10, #0
    mov r11, #0
    mov r12, #0

    // We suppose data to be implented by simulator, save lot of cycles by
    // Not moving it arount. This is only safe if there is a single execution
    // and it is without exceptions
    b _start

//.thumb_func
//data_mv:
//    ldr r0, =_flash_sdata
//    ldr r1, =_sram_sdata
//    ldr r2, =_sram_edata
//
//    /* handle empty data section */
//    cmp r2, r1
//    //beq bss_raz
//    beq _start
//
//.thumb_func
//data_loop:
//    ldr r4, [r0], #4
//    str r4, [r1], #4
//
//    cmp r2, r1
//    bne data_loop
//    b _start

// We make the hypothesis that memory is initialized to 0 and that there is
// only one execution without exceptions or faults so there is not need for
// reset and bss not being overwritten to 0 is safe.
//.thumb_func
//bss_raz:
//    mov r0, #0
//    ldr r1, =_sram_sbss
//    ldr r2, =_sram_ebss
//
//    /* handle empty bss section */
//    cmp r2, r1
//    beq _start
//
//.thumb_func
//bss_loop:
//    str r0, [r1], #4
//
//    cmp r2, r1
//    bne bss_loop
//    b _start

// Move appart the symbols so that they re not wrongly detected during jumps
.space 8
.thumb_func
_start:
    bl main
    b _hang

.space 8
.thumb_func
_hang:
    b .
