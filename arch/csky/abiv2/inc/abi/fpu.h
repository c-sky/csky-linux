// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_FPU_H
#define __ASM_CSKY_FPU_H

#ifndef __ASSEMBLY__ /* C source */

#include <asm/sigcontext.h>
#include <asm/ptrace.h>

int fpu_libc_helper(struct pt_regs *regs);
void fpu_fpe(struct pt_regs *regs);
void __init init_fpu(void);

void save_to_user_fp(struct user_fp *user_fp);
void restore_from_user_fp(struct user_fp *user_fp);

/*
 * Define the fesr bit for fpe handle.
 */
#define  FPE_ILLE  (1 << 16)    /* Illegal instruction  */
#define  FPE_FEC   (1 << 7)     /* Input float-point arithmetic exception */
#define  FPE_IDC   (1 << 5)     /* Input denormalized exception */
#define  FPE_IXC   (1 << 4)     /* Inexact exception */
#define  FPE_UFC   (1 << 3)     /* Underflow exception */
#define  FPE_OFC   (1 << 2)     /* Overflow exception */
#define  FPE_DZC   (1 << 1)     /* Divide by zero exception */
#define  FPE_IOC   (1 << 0)     /* Invalid operation exception */
#define  FPE_REGULAR_EXCEPTION (FPE_IXC | FPE_UFC | FPE_OFC | FPE_DZC | FPE_IOC)

#ifdef CONFIG_OPEN_FPU_IDE
#define IDE_STAT   (1 << 5)
#else
#define IDE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_IXE
#define IXE_STAT   (1 << 4)
#else
#define IXE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_UFE
#define UFE_STAT   (1 << 3)
#else
#define UFE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_OFE
#define OFE_STAT   (1 << 2)
#else
#define OFE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_DZE
#define DZE_STAT   (1 << 1)
#else
#define DZE_STAT   0
#endif

#ifdef CONFIG_OPEN_FPU_IOE
#define IOE_STAT   (1 << 0)
#else
#define IOE_STAT   0
#endif

#else  /* __ASSEMBLY__ */

#include <asm/asm-offsets.h>

.macro  FPU_SAVE_REGS
	/* Save FPU control regs task struct */
	mfcr     r7, cr<1, 2>
	mfcr     r6, cr<2, 2>
	stw      r7, (a3, THREAD_FCR)
	stw      r6, (a3, THREAD_FESR)
	/* Save FPU general regs task struct */
	fmfvrl   r6, vr0
	fmfvrh   r7, vr0
	fmfvrl   r8, vr1
	fmfvrh   r9, vr1
	stw      r6, (a3, THREAD_FPREG + 0)  /* In aviv2: stw can load longer */
	stw      r7, (a3, THREAD_FPREG + 4)
	stw      r8, (a3, THREAD_FPREG + 16)
	stw      r9, (a3, THREAD_FPREG + 20)
	fmfvrl   r6, vr2
	fmfvrh   r7, vr2
	fmfvrl   r8, vr3
	fmfvrh   r9, vr3
	stw      r6, (a3, THREAD_FPREG + 32)
	stw      r7, (a3, THREAD_FPREG + 36)
	stw      r8, (a3, THREAD_FPREG + 48)
	stw      r9, (a3, THREAD_FPREG + 52)
	fmfvrl   r6, vr4
	fmfvrh   r7, vr4
	fmfvrl   r8, vr5
	fmfvrh   r9, vr5
	stw      r6, (a3, THREAD_FPREG + 64)
	stw      r7, (a3, THREAD_FPREG + 68)
	stw      r8, (a3, THREAD_FPREG + 80)
	stw      r9, (a3, THREAD_FPREG + 84)
	fmfvrl   r6, vr6
	fmfvrh   r7, vr6
	fmfvrl   r8, vr7
	fmfvrh   r9, vr7
	stw      r6, (a3, THREAD_FPREG + 96)
	stw      r7, (a3, THREAD_FPREG + 100)
	stw      r8, (a3, THREAD_FPREG + 112)
	stw      r9, (a3, THREAD_FPREG + 116)
	fmfvrl   r6, vr8
	fmfvrh   r7, vr8
	fmfvrl   r8, vr9
	fmfvrh   r9, vr9
	stw      r6, (a3, THREAD_FPREG + 128)
	stw      r7, (a3, THREAD_FPREG + 132)
	stw      r8, (a3, THREAD_FPREG + 144)
	stw      r9, (a3, THREAD_FPREG + 148)
	fmfvrl   r6, vr10
	fmfvrh   r7, vr10
	fmfvrl   r8, vr11
	fmfvrh   r9, vr11
	stw      r6, (a3, THREAD_FPREG + 160)
	stw      r7, (a3, THREAD_FPREG + 164)
	stw      r8, (a3, THREAD_FPREG + 176)
	stw      r9, (a3, THREAD_FPREG + 180)
	fmfvrl   r6, vr12
	fmfvrh   r7, vr12
	fmfvrl   r8, vr13
	fmfvrh   r9, vr13
	stw      r6, (a3, THREAD_FPREG + 192)
	stw      r7, (a3, THREAD_FPREG + 196)
	stw      r8, (a3, THREAD_FPREG + 208)
	stw      r9, (a3, THREAD_FPREG + 212)
	fmfvrl   r6, vr14
	fmfvrh   r7, vr14
	fmfvrl   r8, vr15
	fmfvrh   r9, vr15
	stw      r6, (a3, THREAD_FPREG + 224)
	stw      r7, (a3, THREAD_FPREG + 228)
	stw      r8, (a3, THREAD_FPREG + 240)
	stw      r9, (a3, THREAD_FPREG + 244)
.endm

.macro  FPU_RESTORE_REGS
	/* Save FPU control regs task struct */
	ldw      r6, (a3, THREAD_FCR)
	ldw      r7, (a3, THREAD_FESR)
	mtcr     r6, cr<1, 2>
	mtcr     r7, cr<2, 2>
	/* restore FPU general regs task struct */
	ldw      r6, (a3, THREAD_FPREG + 0)
	ldw      r7, (a3, THREAD_FPREG + 4)
	ldw      r8, (a3, THREAD_FPREG + 16)
	ldw      r9, (a3, THREAD_FPREG + 20)
	fmtvrl   vr0, r6
	fmtvrh   vr0, r7
	fmtvrl   vr1, r8
	fmtvrh   vr1, r9
	ldw      r6, (a3, THREAD_FPREG + 32)
	ldw      r7, (a3, THREAD_FPREG + 36)
	ldw      r8, (a3, THREAD_FPREG + 48)
	ldw      r9, (a3, THREAD_FPREG + 52)
	fmtvrl   vr2, r6
	fmtvrh   vr2, r7
	fmtvrl   vr3, r8
	fmtvrh   vr3, r9
	ldw      r6, (a3, THREAD_FPREG + 64)
	ldw      r7, (a3, THREAD_FPREG + 68)
	ldw      r8, (a3, THREAD_FPREG + 80)
	ldw      r9, (a3, THREAD_FPREG + 84)
	fmtvrl   vr4, r6
	fmtvrh   vr4, r7
	fmtvrl   vr5, r8
	fmtvrh   vr5, r9
	ldw      r6, (a3, THREAD_FPREG + 96)
	ldw      r7, (a3, THREAD_FPREG + 100)
	ldw      r8, (a3, THREAD_FPREG + 112)
	ldw      r9, (a3, THREAD_FPREG + 116)
	fmtvrl   vr6, r6
	fmtvrh   vr6, r7
	fmtvrl   vr7, r8
	fmtvrh   vr7, r9
	ldw      r6, (a3, THREAD_FPREG + 128)
	ldw      r7, (a3, THREAD_FPREG + 132)
	ldw      r8, (a3, THREAD_FPREG + 144)
	ldw      r9, (a3, THREAD_FPREG + 148)
	fmtvrl   vr8, r6
	fmtvrh   vr8, r7
	fmtvrl   vr9, r8
	fmtvrh   vr9, r9
	ldw      r6, (a3, THREAD_FPREG + 160)
	ldw      r7, (a3, THREAD_FPREG + 164)
	ldw      r8, (a3, THREAD_FPREG + 176)
	ldw      r9, (a3, THREAD_FPREG + 180)
	fmtvrl   vr10, r6
	fmtvrh   vr10, r7
	fmtvrl   vr11, r8
	fmtvrh   vr11, r9
	ldw      r6, (a3, THREAD_FPREG + 192)
	ldw      r7, (a3, THREAD_FPREG + 196)
	ldw      r8, (a3, THREAD_FPREG + 208)
	ldw      r9, (a3, THREAD_FPREG + 212)
	fmtvrl   vr12, r6
	fmtvrh   vr12, r7
	fmtvrl   vr13, r8
	fmtvrh   vr13, r9
	ldw      r6, (a3, THREAD_FPREG + 224)
	ldw      r7, (a3, THREAD_FPREG + 228)
	ldw      r8, (a3, THREAD_FPREG + 240)
	ldw      r9, (a3, THREAD_FPREG + 244)
	fmtvrl   vr14, r6
	fmtvrh   vr14, r7
	fmtvrl   vr15, r8
	fmtvrh   vr15, r9
.endm

#endif /* __ASSEMBLY__ */

#endif /* __ASM_CSKY_FPU_H */
