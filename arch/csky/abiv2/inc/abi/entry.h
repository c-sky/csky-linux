// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_ENTRY_H
#define __ASM_CSKY_ENTRY_H

#include <asm/setup.h>
#include <abi/regdef.h>

#define LSAVE_A0       0xc
#define LSAVE_A1       0x10
#define LSAVE_A2       0x14
#define LSAVE_A3       0x18

#define KSPTOUSP
#define USPTOKSP

.macro GET_USP rx
	mfcr	\rx, cr<14, 1>
.endm

.macro SET_USP rx
	mtcr	\rx, cr<14, 1>
.endm

.macro INCTRAP	rx
	addi	\rx, 4
.endm

.macro SAVE_ALL
	subi    sp,  144
	stw     a0, (sp, 4)
	stw     a0, (sp, 12)
	stw     a1, (sp, 16)
	stw     a2, (sp, 20)
	stw     a3, (sp, 24)
	stw     r4, (sp, 28)
	stw     r5, (sp, 32)
	stw     r6, (sp, 36)
	stw     r7, (sp, 40)
	stw     r8, (sp, 44)
	stw     r9, (sp, 48)
	stw     r10, (sp, 52)
	stw     r11, (sp, 56)
	stw     r12, (sp, 60)
	stw     r13, (sp, 64)
	stw     r15, (sp, 68)
	addi    sp, 72
	stm     r16-r31,(sp)
#ifdef CONFIG_CPU_HAS_HILO
	mfhi    r22
	mflo    r23
	stw     r22, (sp, 64)
        stw     r23, (sp, 68)
#endif
	subi    sp,  72

	mfcr    r22, epsr
	stw     r22, (sp, 8)
	mfcr    r22, epc
	stw     r22, (sp)
.endm
.macro SAVE_ALL_TRAP
	SAVE_ALL
	INCTRAP	r22
	stw     r22, (sp)
.endm

.macro	RESTORE_ALL
	psrclr  ie
	ldw     a0, (sp)
	mtcr    a0, epc
	ldw     a0, (sp, 8)
	mtcr    a0, epsr
	addi    sp, 12
#ifdef CONFIG_CPU_HAS_HILO
	ldw     a0, (sp, 124)
	ldw     a1, (sp, 128)
	mthi    a0
	mtlo    a1
#endif
	ldw     a0, (sp, 0)
	ldw     a1, (sp, 4)
	ldw     a2, (sp, 8)
	ldw     a3, (sp, 12)
	ldw     r4, (sp, 16)
	ldw     r5, (sp, 20)
	ldw     r6, (sp, 24)
	ldw     r7, (sp, 28)
	ldw     r8, (sp, 32)
	ldw     r9, (sp, 36)
	ldw     r10, (sp, 40)
	ldw     r11, (sp, 44)
	ldw     r12, (sp, 48)
	ldw     r13, (sp, 52)
	ldw     r15, (sp, 56)
	addi    sp, 60
	ldm     r16-r31,(sp)
	addi    sp,  72
1:
	rte
.endm

.macro SAVE_SWITCH_STACK
        subi    sp, 64
        stm     r4-r11,(sp)
        stw     r15, (sp, 32)
        stw     r16, (sp, 36)
        stw     r17, (sp, 40)
        stw     r26, (sp, 44)
        stw     r27, (sp, 48)
        stw     r28, (sp, 52)
        stw     r29, (sp, 56)
        stw     r30, (sp, 60)
.endm

.macro RESTORE_SWITCH_STACK
        ldm     r4-r11,(sp)
        ldw     r15, (sp, 32)
        ldw     r16, (sp, 36)
        ldw     r17, (sp, 40)
        ldw     r26, (sp, 44)
        ldw     r27, (sp, 48)
        ldw     r28, (sp, 52)
        ldw     r29, (sp, 56)
        ldw     r30, (sp, 60)
        addi    sp, 64
.endm

/* MMU registers operators. */
.macro RD_MIR	rx
	mfcr    \rx, cr<0, 15>
.endm

.macro RD_MEH	rx
	mfcr    \rx, cr<4, 15>
.endm

.macro RD_MCIR	rx
	mfcr    \rx, cr<8, 15>
.endm

.macro RD_PGDR  rx
        mfcr    \rx, cr<29, 15>
.endm

.macro WR_MEH	rx
	mtcr    \rx, cr<4, 15>
.endm

.macro WR_MCIR	rx
	mtcr    \rx, cr<8, 15>
.endm

#endif /* __ASM_CSKY_ENTRY_H */
