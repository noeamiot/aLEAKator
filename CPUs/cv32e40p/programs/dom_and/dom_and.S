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
    lw  x30, 0(x11)
	la  x12, as0
    lw  x18, 0(x12)
	la  x12, bs1
    lw  x29, 0(x12)
	and	x12,x18,x29
	xor	x12,x12,x30 # a0 and b1 xor m in x14
	la  x28, blk

	la  x15, as1
    lw  x16, 0(x28)
    lw  x16, 0(x15)
    nop // vwg

    and	x6,x16,x29 # a1b1 in x16
    xor x5, x5, x5 // vwg

    xor	x12,x6,x12 # res0 in x12
//    mv x6,x0
    la  x6, res0
    sw  x12, 0(x6)
    nop // vwg
    nop // vwg
    sw x0, 0(x28)
    mv x12,x0
    mv x29,x0

    la  x15, bs0
    lw  x5, 0(x15)
    nop // twg
    nop // vwg

	and	x20,x16,x5
    mv x16,x0 // vwg

    nop // vwg
    xor x0, x0, x0
    xor x0, x0, x0 // twg

    and	x27,x5,x18 # a0b0 in x17
    mv x5,x0
    mv x18,x0
    nop
    nop

	xor	x29,x30,x20 # a1 and b0 xor m in x15
    mv x30, x0 // vwg
    mv x20, x0 // vwg
    nop // vwg
    nop // vwg
    xor	x13,x29,x27 # res1 in x13

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
	.type	blk, @object
	.size	blk, 4
blk:
	.zero	4
	.type	bs0, @object
	.size	bs0, 4
	.type	bs0, @object
	.size	bs0, 4
bs0:
	.word	0xf0000000
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
