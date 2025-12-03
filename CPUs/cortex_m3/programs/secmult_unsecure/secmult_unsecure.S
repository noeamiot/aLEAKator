    .text
    .align 1
    .global main
    .syntax unified
    .thumb
    .thumb_func
    .type main, %function
main:
	push	{r4, r5, r6, r7, r8, r9, r10, fp, lr}
	sub	sp, sp, #12
	str	r1, [sp]

	ldr	r2, .L10
	ldr	r1, .L11
	ldrb	r9, [r2]	@ zero_extendqisi2
	ldrsb	r3, [r2]
	ldr	r4, .L12
	ldrb	r2, [r1]	@ zero_extendqisi2
	ldr	r1, .L13
	asr.W	r3, r3, #7 @ Mismatch
	ldrb	r0, [r1]	@ zero_extendqisi2
	ldrsb	r1, [r4]
	ldrb	r4, [r4]	@ zero_extendqisi2
	asr.W	r1, r1, #7 @ Mismatch
	and	r1, r1, #27
	eor	r1, r1, r4, lsl #1
	sbfx	ip, r1, #7, #1
	and	ip, ip, #27
	uxtb	r1, r1
	eor	ip, ip, r1, lsl #1
	ldr	r5, .L14
	sbfx	r6, ip, #7, #1
	and	r3, r3, #27
	eor	r3, r3, r9, lsl #1
	sbfx	r7, r2, #0, #1
	uxtb	ip, ip
	and	r6, r6, #27
	ldrb	r10, [r5]	@ zero_extendqisi2
	uxtb	r8, r3
	eor	fp, r6, ip, lsl #1
	sbfx	r5, r2, #1, #1
	and	r6, r7, r9
	and.W	r7, r7, r4 @ Mismatch
	str	r7, [sp, #4]
	sbfx	r3, r3, #7, #1
	and	r7, r5, r8
	eor.W	r6, r6, r7 @ Mismatch
	and	r3, r3, #27
	ldr	r7, [sp, #4]
	eor	r3, r3, r8, lsl #1
	and.W	r5, r5, r1 @ Mismatch
	sbfx	lr, r3, #7, #1
	eor.W	r7, r7, r5 @ Mismatch
	sbfx	r5, r0, #0, #1
	uxtb	r3, r3
	and.W	r4, r4, r5 @ Mismatch
	eor	r6, r10, r6
	and	r5, r5, r9
	and	lr, lr, #27
	sbfx	r9, r0, #1, #1
	eor	lr, lr, r3, lsl #1
	and	r1, r9, r1
	eor.W	r6, r6, r4 @ Mismatch
	and	r9, r9, r8
	eor	r4, r6, r1
	eor	r9, r5, r9
	sbfx	r1, lr, #7, #1
	sbfx	r5, r2, #2, #1
	sbfx	r6, fp, #7, #1
	uxtb	r8, fp
	uxtb	lr, lr
	and	fp, r5, r3
	eor	r10, r10, r9
	and	r5, r5, ip
	sbfx	r9, r0, #2, #1
	and	r1, r1, #27
	and	r6, r6, #27
	eor	r1, r1, lr, lsl #1
	eor	r6, r6, r8, lsl #1
	eor	r4, r4, fp
	and	r3, r9, r3
	eor.W	r7, r7, r5 @ Mismatch
	and	ip, r9, ip
	sbfx	r5, r2, #3, #1
	sbfx	r9, r0, #3, #1
	uxtb	fp, r1
	eor	ip, r4, ip
	eor	r10, r10, r3
	sbfx	r4, r1, #7, #1
	and	r3, r5, lr
	sbfx	r1, r6, #7, #1
	and	lr, r9, lr
	uxtb	r6, r6
	and	r5, r5, r8
	eor	ip, ip, r3
	and	r8, r9, r8
	eor	r10, r10, lr
	and	r4, r4, #27
	sbfx	lr, r2, #4, #1
	and	r1, r1, #27
	eor	r1, r1, r6, lsl #1
	eor	r4, r4, fp, lsl #1
	eor.W	r7, r7, r5 @ Mismatch
	eor	ip, ip, r8
	sbfx	r5, r0, #4, #1
	and	r8, lr, fp
	eor	ip, ip, r8
	and	lr, lr, r6
	sbfx	r3, r4, #7, #1
	and.W	r6, r6, r5 @ Mismatch
	sbfx	r9, r1, #7, #1
	and	r5, r5, fp
	uxtb	r4, r4
	eor	r7, lr, r7
	eor	r6, ip, r6
	sbfx	lr, r2, #5, #1
	sbfx	ip, r0, #5, #1
	eor	r10, r10, r5
	and	r3, r3, #27
	uxtb	r5, r1
	and	r9, r9, #27
	eor	r9, r9, r5, lsl #1
	and	r8, lr, r4
	eor	r3, r3, r4, lsl #1
	and	r4, ip, r4
	and	r1, ip, r5
	and	lr, lr, r5
	uxtb	ip, r3
	sbfx	r5, r2, #6, #1
	eor	r6, r6, r8
	eor	r10, r10, r4
	sbfx	r3, r3, #7, #1
	sbfx	r4, r9, #7, #1
	eor	r8, r6, r1
	eor	r7, lr, r7
	sbfx	r6, r0, #6, #1
	uxtb	lr, r9
	and	r1, r5, ip
	and	r3, r3, #27
	sbfx	r2, r2, #7, #1
	and	r4, r4, #27
	eor	r3, r3, ip, lsl #1
	eor	r4, r4, lr, lsl #1
	and	r5, r5, lr
	uxtb	r2, r2
	and	lr, r6, lr
	eor	r1, r8, r1
	sbfx	r0, r0, #7, #1
	uxtb	r0, r0
	eor.W	r7, r7, r5 @ Mismatch
	eor	r1, r1, lr
	and	r5, r3, r2
	eor.W	r1, r1, r5 @ Mismatch
	and.W	r2, r2, r4 @ Mismatch
	and.W	r4, r4, r0 @ Mismatch
	and	r7, r7, #255
	eor.W	r4, r4, r1 @ Mismatch
	and.W	r0, r0, r3 @ Mismatch
	ldr	r3, .L15
	uxtb	r4, r4
	and	r6, r6, ip
	eor.W	r2, r2, r7 @ Mismatch
	eor.W	r2, r2, r4 @ Mismatch
	ldr	r5, .L16
	eor	r10, r10, r6
	strb	r4, [r3]
	ldr	r4, .L17
	eor	r0, r10, r0
	strb	r0, [r5]
	strb	r2, [r4]
	nop


@	ldrb	r3, [r5]	@ zero_extendqisi2
@	ldr	r2, [sp]
@	str	r3, [r2]
@
@	ldrb	r3, [r4]	@ zero_extendqisi2
@	str	r3, [r2, #4]

	add	sp, sp, #12
	pop	{r4, r5, r6, r7, r8, r9, r10, fp, pc}
    bx lr

	.align	2
.L10:
	.word	.LANCHOR0
.L11:
	.word	.LANCHOR3
.L12:
	.word	.LANCHOR2
.L13:
	.word	.LANCHOR1
.L14:
	.word	.LANCHOR4
.L15:
	.word	.LANCHOR5
.L16:
	.word	.LANCHOR6
.L17:
	.word	.LANCHOR7
.LFE2:
	.size	test_vector, .-test_vector
	.global	m01
	.global	b0
	.global	a0
@	.global	b
@	.global	a
	.global	m10
	.global	c1
	.global	c0
	.global	b1
	.global	a1
@	.section	.data.a,"aw",%nobits
@	.type	a, %object
@	.size	a, 1
@a:
@	.space	1
	.section	.data.a0,"aw",%progbits
	.set	.LANCHOR0,. + 0
	.type	a0, %object
	.size	a0, 1
a0:
	.byte 20
	.section	.data.a1,"aw",%progbits
	.set	.LANCHOR2,. + 0
	.type	a1, %object
	.size	a1, 1
a1:
	.byte 25
@	.section	.data.b,"aw",%nobits
@	.type	b, %object
@	.size	b, 1
@b:
@	.space	1
	.section	.data.b0,"aw",%progbits
	.set	.LANCHOR1,. + 0
	.type	b0, %object
	.size	b0, 1
b0:
	.byte 230
	.section	.data.b1,"aw",%progbits
	.set	.LANCHOR3,. + 0
	.type	b1, %object
	.size	b1, 1
b1:
	.byte 11
	.section	.data.c0,"aw",%nobits
	.set	.LANCHOR6,. + 0
	.type	c0, %object
	.size	c0, 1
c0:
	.space	1
	.section	.data.c1,"aw",%nobits
	.set	.LANCHOR7,. + 0
	.type	c1, %object
	.size	c1, 1
c1:
	.space	1
	.section	.data.m01,"aw",%progbits
	.set	.LANCHOR4,. + 0
	.type	m01, %object
	.size	m01, 1
m01:
	.byte 154
	.section	.data.m10,"aw",%nobits
	.set	.LANCHOR5,. + 0
	.type	m10, %object
	.size	m10, 1
m10:
	.space	1
