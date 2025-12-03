	.file	"dom_and.c"
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
	la  x11, m
    lw  x14, 0(x11)
	la  x11, as0
    lw  x12, 0(x11)
	la  x11, bs1
    lw  x13, 0(x11)
	and	x11,x12,x13
	xor	x11,x11,x14 # a0 and b1 xor m in x14

	la  x15, as1
    lw  x16, 0(x15)
	la  x15, bs0
    lw  x17, 0(x15)
	and	x15,x16,x17
	xor	x15,x15,x14 # a1 and b0 xor m in x15

    # Prevent transition nop
    and x0, x0, x0
    and x0, x0, x0

    and	x16,x16,x13 # a1b1 in x16

    # Prevent transition nop
    and x0, x0, x0
    and x0, x0, x0

    and	x17,x17,x12 # a0b0 in x17

    # Prevent transition nop
    and x12, x0, x0 // Clear x12
    la  x14, res0
    and x0, x0, x0
    and x0, x0, x0

    xor	x12,x16,x11 # res0 in x12
    //sw  x0, 0(x14) // Clear LSU before storing result
    sw  x12, 0(x14)
    and x0, x0, x0

    # Prevent transition nop
    and x13, x0, x0 // Clear x13
    and x0, x0, x0

    xor	x13,x17,x15 # res1 in x13

    la  x15, res1
    sw  x13, 0(x15)

	li	a0,0
	ret
	.cfi_endproc
.LFE0:
	.size	main, .-main
	.globl	res1
	.globl	res0
	.globl	m
	.globl	bs1
	.globl	bs0
	.globl	as1
	.globl	as0
	.section	.sdata,"aw"
	.align	2
	.type	res1, @object
	.size	res1, 4
res1:
	.zero	4
	.type	res0, @object
	.size	res0, 4
res0:
	.zero	4
	.type	bs0, @object
	.size	bs0, 4
bs0:
	.word	0xF0000000
	.type	as1, @object
	.size	as1, 4
as1:
	.word   0x0000000F
	.align	2
	.type	m, @object
	.size	m, 4
m:
	.word	0x01234567
	.type	bs1, @object
	.size	bs1, 4
bs1:
	.word	0xF5FFFFFF
	.type	as0, @object
	.size	as0, 4
as0:
	.word	0xFAFFFFFF
	.ident	"GCC: () 14.2.0"
	.section	.note.GNU-stack,"",@progbits
