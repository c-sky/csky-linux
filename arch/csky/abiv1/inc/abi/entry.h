// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_ENTRY_H
#define __ASM_CSKY_ENTRY_H

#include <asm/setup.h>
#include <abi/regdef.h>

/*
 * Stack layout for exception (sp=r0):
 *
 *	 0(sp) - pc
 *	 4(sp) - orig_a0
 *	 8(sp) - sr
 *	 C(sp) - a0/r2
 *	10(sp) - a1/r3
 *	14(sp) - a2/r4
 *	18(sp) - a3/r5
 *	1C(sp) - r6
 *	20(sp) - r7
 *	24(sp) - r8
 *	28(sp) - r9
 *	2C(sp) - r10
 *	30(sp) - r11
 *	34(sp) - r12
 *	38(sp) - r13
 *	3C(sp) - r14
 *	40(sp) - r1
 *	44(sp) - r15
 */

#define LSAVE_A0	0xc
#define LSAVE_A1	0x10
#define LSAVE_A2	0x14
#define LSAVE_A3	0x18
#define LSAVE_A4	0x1C
#define LSAVE_A5	0x20

.macro USPTOKSP
	mtcr	sp, ss1
	mfcr	sp, ss0
.endm

.macro KSPTOUSP
	mtcr	sp, ss0
	mfcr	sp, ss1
.endm

.macro GET_USP rx
	mfcr	\rx, ss1
.endm

.macro SET_USP rx
	mtcr	\rx, ss1
.endm

.macro INCTRAP	rx
	addi	\rx, 2
.endm

/*
 * SAVE_ALL: save the pt_regs to the stack.
 */
.macro	SAVE_ALL
	mtcr    r13, ss2
	mfcr    r13, epsr
	btsti   r13, 31
	bt      1f
	USPTOKSP
1:
	subi    sp, 32
	subi    sp, 32
	stw     r13,	(sp, 0)
	mfcr    r13,	ss2
	stw     a0,	(sp, 4)
	stw     a1,	(sp, 8)
	stw     a2,	(sp, 12)
	stw     a3,	(sp, 16)
	stw     r6,	(sp, 20)
	stw     r7,	(sp, 24)
	stw     r8,	(sp, 28)
	stw     r9,	(sp, 32)
	stw     r10,	(sp, 36)
	stw     r11,	(sp, 40)
	stw     r12,	(sp, 44)
	stw     r13,	(sp, 48)
	stw     r14,	(sp, 52)
	stw     r1,	(sp, 56)
	stw     r15,	(sp, 60)

	subi    sp,	8
	stw     a0,	(sp, 4)
	mfcr    r13,	epc
	stw     r13,	(sp)
.endm

.macro SAVE_ALL_TRAP
	SAVE_ALL
	INCTRAP r13
	stw     r13,	(sp)
.endm

.macro	RESTORE_ALL
	psrclr  ie
	ldw     a0, (sp)
	mtcr    a0, epc
	ldw     a0, (sp, 8)
	mtcr    a0, epsr
	btsti   a0, 31

	addi    sp, 12
	ldw     a0, (sp, 0)
	ldw     a1, (sp, 4)
	ldw     a2, (sp, 8)
	ldw     a3, (sp, 12)
	ldw     r6, (sp, 16)
	ldw     r7, (sp, 20)
	ldw     r8, (sp, 24)
	ldw     r9, (sp, 28)
	ldw     r10, (sp, 32)
	ldw     r11, (sp, 36)
	ldw     r12, (sp, 40)
	ldw     r13, (sp, 44)
	ldw     r14, (sp, 48)
	ldw     r1, (sp, 52)
	ldw     r15, (sp, 56)
	addi    sp, 32
	addi    sp, 28

	bt      1f
	KSPTOUSP
1:
	rte
.endm

.macro SAVE_SWITCH_STACK
	subi    sp, 32
	stm     r8-r15,(sp)
.endm

.macro RESTORE_SWITCH_STACK
        ldm     r8-r15,(sp)
        addi    sp, 32
.endm

/* MMU registers operators. */
.macro RD_MIR	rx
	cprcr   \rx, cpcr0
.endm

.macro RD_MEH	rx
	cprcr   \rx, cpcr4
.endm

.macro RD_MCIR	rx
	cprcr   \rx, cpcr8
.endm

.macro RD_PGDR  rx
        cprcr   \rx, cpcr29
.endm

.macro WR_MEH	rx
	cpwcr   \rx, cpcr4
.endm

.macro WR_MCIR	rx
	cpwcr   \rx, cpcr8
.endm

#endif /* __ASM_CSKY_ENTRY_H */
