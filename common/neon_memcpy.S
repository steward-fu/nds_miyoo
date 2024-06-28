/*
   Copyright (C) 2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   Contributed by MontaVista Software, Inc. (written by Nicolas Pitre)
   NEON code contributed by Nokia Corporation (written by Siarhei Siamashka)

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.
*/

        .text
        .fpu    neon

        .global neon_memcpy
        .type neon_memcpy, %function
        .align 4
/*
 * Unaligned memory accesses are avoided, even though they
 * would be a bit faster. This is done in order to avoid any
 * potential problems if SCTLR.A bit is set or strongly
 * ordered/device memory is accessed.
 */
neon_memcpy:
		cmp	r2, #16
		bge	3f

		/* Do small memory copies (up to 15 bytes) using ARM */
		push	{r0, lr}
		subs	r2, r2, #2
		blt	2f
1:		ldrb	r3, [r1], #1
		ldrb	lr, [r1], #1
		subs	r2, r2, #2
		strb	r3, [r0], #1
		strb	lr, [r0], #1
		bge	1b
2:		cmp	r2, #-1
		ldreqb	r3, [r1], #1
		streqb	r3, [r0], #1
		pop	{r0, pc}
3:
		/* Do bigger memory copies using NEON instructions */
		mov	ip, r0
		tst	r0, #1
		beq	1f
		vld1.8	{d0[0]}, [r1]!
		vst1.8	{d0[0]}, [ip]!
		sub	r2, r2, #1
1:
		tst	ip, #2
		beq	1f
		vld2.8	{d0[0], d1[0]}, [r1]!
		vst2.8	{d0[0], d1[0]}, [ip]!
		sub	r2, r2, #2
1:
		tst	ip, #4
		beq	1f
		vld4.8	{d0[0], d1[0], d2[0], d3[0]}, [r1]!
		vst4.8	{d0[0], d1[0], d2[0], d3[0]}, [ip, :32]!
		sub	r2, r2, #4
1:
		tst	ip, #8
		beq	1f
		vld1.8	{d0}, [r1]!
		vst1.8	{d0}, [ip, :64]!
		sub	r2, r2, #8
1:
		subs	r2, r2, #32
		blt	3f
		mov	r3, #32
1:
		vld1.8	{d0-d3}, [r1]!
		cmp	r3, #(320 - 32)
		pld	[r1, r3]
		addle	r3, r3, #32
		sub	r2, r2, #32
		vst1.8	{d0-d3}, [ip, :128]!
		cmp	r2, r3
		bge	1b
		cmp	r2, #0
		blt	3f
1:
		vld1.8	{d0-d3}, [r1]!
		subs	r2, r2, #32
		vst1.8	{d0-d3}, [ip, :128]!
		bge	1b
3:
		tst	r2, #16
		beq	1f
		vld1.8	{d0, d1}, [r1]!
		vst1.8	{d0, d1}, [ip, :128]!
1:
		tst	r2, #8
		beq	1f
		vld1.8	{d0}, [r1]!
		vst1.8	{d0}, [ip, :64]!
1:
		tst	r2, #4
		beq	1f
		vld4.8	{d0[0], d1[0], d2[0], d3[0]}, [r1]!
		vst4.8	{d0[0], d1[0], d2[0], d3[0]}, [ip, :32]!
1:
		tst	r2, #2
		beq	1f
		vld2.8	{d0[0], d1[0]}, [r1]!
		vst2.8	{d0[0], d1[0]}, [ip]!
1:
		tst	r2, #1
		beq	1f
		vld1.8	{d0[0]}, [r1]!
		vst1.8	{d0[0]}, [ip]!
1:
		bx	lr
