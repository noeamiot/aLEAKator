	.file	"secmult.c"
	.option nopic
	.attribute arch, "rv32i2p1_c2p0"
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.text
	.align	1
	.globl	gmul
	.type	gmul, @function
gmul:
.LFB0:
	//mv	a5,a0
    andi a5, a5, 0x0
    andi a5, a0, 0xFF
	li	a3,8
	li	a0,0
.L2:
	slli	a4,a1,31
	srai	a4,a4,31
	and	a4,a5,a4
	xor	a0,a0,a4
	slli	a5,a5,24
	srai	a5,a5,24
	srai	a4,a5,7
	andi	a4,a4,27
	slli	a5,a5,1
	xor	a5,a4,a5
	andi	a5,a5,0xff
	srli	a1,a1,1
	addi	a3,a3,-1
	bne	a3,zero,.L2
    // Prevent glitches
    andi a4, a0, 0x0
	ret
.LFE0:
	.size	gmul, .-gmul
	.align	1
	.globl	secmult
	.type	secmult, @function
secmult:
.LFB1:
	addi	sp,sp,-32
	sw	ra,28(sp)
	sw	s0,24(sp)
	sw	t6,20(sp)
	sw	s2,16(sp)
	sw	s3,12(sp)
	sw	s4,8(sp)
	sw	t5,4(sp)




    la  a5, a0
	lw	s4,0(a5) #a0 in s4
    la  a5, b0
	lw	t4,0(a5) # b0
    and  a1, t4, 0xFF
//    la  a5, b0
//	lw	t4,0(a5)
    la  a5, m01
	lw	s3,0(a5) # m01 in s3
    andi a0, s4, 0xFF
	call	gmul
//    jal x1, gmul
    xor a0, a0, s3
    la  a5, c0
	sw	a0,0(a5)
    // Clear res before loading new shares
    and a0, a0, 0x0

    la  a5, b1
	lw	s2,0(a5) # b1 in s2
    andi a1, s2, 0xFF
    andi a0, s4, 0xFF
	call	gmul
//    jal x1, gmul
    xor s3, a0, s3

    // Clear a0 share as it is not useful anymore
    and s4, s4, 0x0
    // Prevent transition wog on a0
    and a0, a0, 0x0

    // Compute gmul (a1,b1)
    la  s4, a1
	lw	s4,0(s4) # a1 in s4
    andi a0, s4, 0xFF
    andi a1, s2, 0xFF
    and  s2, s2, 0x0 // Clear b1, not used anymore (g)
    and  a5, a5, 0x0
	call	gmul
//    jal x1, gmul
    and a7, a0, 0xFF // move result, we will xor it with m10 later

    and a0, a0, 0x0 // Clear operands
    and a1, a1, 0x0 // Clear operands
    

    // Stabilize destination a1 for when the share b0 will be loaded
//    la  a5, b0
//	lw	a1,0(a5)
    // Stabilize before moving the share here
    and a1, t3, t3
    and  a1,t4, 0xFF 
    and a0, s4, 0xFF

    and s4, s4, 0x0 //clear a1 share
    and t4, t4, 0x0 //clear 

	call	gmul
//    jal x1, gmul
    xor s4, s3, a0

    // Prevent transition
    and a0, a0, 0x0
    and a0, a0, 0x0

    // Transition with glitches, stabilize destination
    and t5, t5, t5
    xor t5, s4, a7

    la  a5, c1
	sw	t5,0(a5)



	lw	ra,28(sp)
	lw	s0,24(sp)
	lw	t6,20(sp)
	lw	s2,16(sp)
	lw	s3,12(sp)
	lw	s4,8(sp)
	lw	t5,4(sp)
	addi	sp,sp,32
	jr	ra
.LFE1:
	.size	secmult, .-secmult
	.align	1
	.globl	main
	.type	main, @function
main:
.LFB2:
	addi	sp,sp,-16
	sw	ra,12(sp)
//	call	secmult
    jal x1, secmult
	li	a0,0
	lw	ra,12(sp)
	addi	sp,sp,16
	jr	ra
.LFE2:
	.size	main, .-main
	.globl	out
	.globl	in
	.globl	m01
	.globl	b0
	.globl	a0
	.globl	m10
	.globl	c1
	.globl	c0
	.globl	b1
	.globl	a1
	.bss
	.align	2
	.type	out, @object
	.size	out, 16
out:
	.zero	16
	.type	in, @object
	.size	in, 16
in:
	.zero	16
	.section	.sbss,"aw",@nobits
	.align	2
	.type	m10, @object
	.size	m10, 4
m10:
	.zero	4
	.type	c1, @object
	.size	c1, 4
c1:
	.zero	4
	.type	c0, @object
	.size	c0, 4
c0:
	.zero	4
	.section	.sdata,"aw"
	.align	2
	.type	m01, @object
	.size	m01, 4
m01:
	.word	154
	.type	b0, @object
	.size	b0, 4
b0:
	.word	230
	.type	a0, @object
	.size	a0, 4
a0:
	.word	20
	.type	b1, @object
	.size	b1, 4
b1:
	.word	11
	.type	a1, @object
	.size	a1, 4
a1:
	.word	25
	.ident	"GCC: () 14.2.0"
	.section	.note.GNU-stack,"",@progbits
