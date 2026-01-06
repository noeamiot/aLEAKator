	.option nopic
	.attribute arch, "rv32i2p1"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.section	.text.startup,"ax",@progbits
	.align	2
	.globl	main
	.type	main, @function
main:
.LFB0:
	.cfi_startproc

    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
#    li x12, 0xF0F0F0F0
#    li x13, 0x0F0F0F0F
#
#    and x10, x8, x12
#    and x11, x9, x13
#
#    and x10, x8, x12
#    and x11, x9, x13

    li x12, 0xFFFFFFFF
    li x13, 0xFFFFFFFF

#    and x10, x8, x12
#    and x10, x8, x13
#    and x11, x9, x12
#    and x11, x9, x13

    and x10, x8, x12
    and x11, x9, x13

    # test transition in reg
#    and x11, x8, x12
#    and x11, x9, x13

    nop
    nop
    nop

//	li	a0,0
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
