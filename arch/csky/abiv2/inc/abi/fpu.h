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

int restore_fpu_state(struct sigcontext *sc);
int save_fpu_state(struct sigcontext *sc, struct pt_regs *regs);

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

#define FMFS_FPU_REGS(frw, frx, fry, frz) \
	"fmfs   %0, "#frw" \n" \
	"fmfs   %1, "#frx" \n" \
	"fmfs   %2, "#fry" \n" \
	"fmfs   %3, "#frz" \n"

#define FMTS_FPU_REGS(frw, frx, fry, frz) \
	"fmts   %0, "#frw" \n" \
	"fmts   %1, "#frx" \n" \
	"fmts   %2, "#fry" \n" \
	"fmts   %3, "#frz" \n"

#define FMFVR_FPU_REGS(vrx, vry) \
	"fmfvrl %0, "#vrx" \n" \
	"fmfvrh %1, "#vrx" \n" \
	"fmfvrl %2, "#vry" \n" \
	"fmfvrh %3, "#vry" \n"

#define FMTVR_FPU_REGS(vrx, vry) \
	"fmtvrl "#vrx", %0 \n" \
	"fmtvrh "#vrx", %1 \n" \
	"fmtvrl "#vry", %2 \n" \
	"fmtvrh "#vry", %3 \n"

#define STW_FPU_REGS(a, b, c, d) \
	"stw    %0, (%4, "#a") \n" \
	"stw    %1, (%4, "#b") \n" \
	"stw    %2, (%4, "#c") \n" \
	"stw    %3, (%4, "#d") \n"

#define LDW_FPU_REGS(a, b, c, d) \
	"ldw    %0, (%4, "#a") \n" \
	"ldw    %1, (%4, "#b") \n" \
	"ldw    %2, (%4, "#c") \n" \
	"ldw    %3, (%4, "#d") \n"

static inline void save_fp_to_thread(unsigned long  * fpregs,
	   unsigned long * fcr, unsigned long * fsr, unsigned long * fesr)
{
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;

	local_save_flags(flg);

	asm volatile(
		"mfcr    %0, cr<1, 2> \n"
		"mfcr    %1, cr<2, 2> \n"
		:"+r"(tmp1),"+r"(tmp2));

	*fcr = tmp1;
	/* not use in fpuv2 */
	*fsr = 0;
	*fesr = tmp2;
	asm volatile(
		FMFVR_FPU_REGS(vr0, vr1)
		STW_FPU_REGS(0, 4, 8, 12)
		FMFVR_FPU_REGS(vr2, vr3)
		STW_FPU_REGS(16, 20, 24, 28)
		FMFVR_FPU_REGS(vr4, vr5)
		STW_FPU_REGS(32, 36, 40, 44)
		FMFVR_FPU_REGS(vr6, vr7)
		STW_FPU_REGS(48, 52, 56, 60)
		"addi    %4, 32 \n"
		"addi    %4, 32 \n"
		FMFVR_FPU_REGS(vr8, vr9)
		STW_FPU_REGS(0, 4, 8, 12)
		FMFVR_FPU_REGS(vr10, vr11)
		STW_FPU_REGS(16, 20, 24, 28)
		FMFVR_FPU_REGS(vr12, vr13)
		STW_FPU_REGS(32, 36, 40, 44)
		FMFVR_FPU_REGS(vr14, vr15)
		STW_FPU_REGS(48, 52, 56, 60)
		:"=a"(tmp1),"=a"(tmp2),"=a"(tmp3),
		"=a"(tmp4),"+a"(fpregs));
	local_irq_restore(flg);
}

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
	stw      r8, (a3, THREAD_FPREG + 8)
	stw      r9, (a3, THREAD_FPREG + 12)
	fmfvrl   r6, vr2
	fmfvrh   r7, vr2
	fmfvrl   r8, vr3
	fmfvrh   r9, vr3
	stw      r6, (a3, THREAD_FPREG + 16)
	stw      r7, (a3, THREAD_FPREG + 20)
	stw      r8, (a3, THREAD_FPREG + 24)
	stw      r9, (a3, THREAD_FPREG + 28)
	fmfvrl   r6, vr4
	fmfvrh   r7, vr4
	fmfvrl   r8, vr5
	fmfvrh   r9, vr5
	stw      r6, (a3, THREAD_FPREG + 32)
	stw      r7, (a3, THREAD_FPREG + 36)
	stw      r8, (a3, THREAD_FPREG + 40)
	stw      r9, (a3, THREAD_FPREG + 44)
	fmfvrl   r6, vr6
	fmfvrh   r7, vr6
	fmfvrl   r8, vr7
	fmfvrh   r9, vr7
	stw      r6, (a3, THREAD_FPREG + 48)
	stw      r7, (a3, THREAD_FPREG + 52)
	stw      r8, (a3, THREAD_FPREG + 56)
	stw      r9, (a3, THREAD_FPREG + 60)
	fmfvrl   r6, vr8
	fmfvrh   r7, vr8
	fmfvrl   r8, vr9
	fmfvrh   r9, vr9
	stw      r6, (a3, THREAD_FPREG + 64)
	stw      r7, (a3, THREAD_FPREG + 68)
	stw      r8, (a3, THREAD_FPREG + 72)
	stw      r9, (a3, THREAD_FPREG + 76)
	fmfvrl   r6, vr10
	fmfvrh   r7, vr10
	fmfvrl   r8, vr11
	fmfvrh   r9, vr11
	stw      r6, (a3, THREAD_FPREG + 80)
	stw      r7, (a3, THREAD_FPREG + 84)
	stw      r8, (a3, THREAD_FPREG + 88)
	stw      r9, (a3, THREAD_FPREG + 92)
	fmfvrl   r6, vr12
	fmfvrh   r7, vr12
	fmfvrl   r8, vr13
	fmfvrh   r9, vr13
	stw      r6, (a3, THREAD_FPREG + 96)
	stw      r7, (a3, THREAD_FPREG + 100)
	stw      r8, (a3, THREAD_FPREG + 104)
	stw      r9, (a3, THREAD_FPREG + 108)
	fmfvrl   r6, vr14
	fmfvrh   r7, vr14
	fmfvrl   r8, vr15
	fmfvrh   r9, vr15
	stw      r6, (a3, THREAD_FPREG + 112)
	stw      r7, (a3, THREAD_FPREG + 116)
	stw      r8, (a3, THREAD_FPREG + 120)
	stw      r9, (a3, THREAD_FPREG + 124)
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
	ldw      r8, (a3, THREAD_FPREG + 8)
	ldw      r9, (a3, THREAD_FPREG + 12)
	fmtvrl   vr0, r6
	fmtvrh   vr0, r7
	fmtvrl   vr1, r8
	fmtvrh   vr1, r9
	ldw      r6, (a3, THREAD_FPREG + 16)
	ldw      r7, (a3, THREAD_FPREG + 20)
	ldw      r8, (a3, THREAD_FPREG + 24)
	ldw      r9, (a3, THREAD_FPREG + 28)
	fmtvrl   vr2, r6
	fmtvrh   vr2, r7
	fmtvrl   vr3, r8
	fmtvrh   vr3, r9
	ldw      r6, (a3, THREAD_FPREG + 32)
	ldw      r7, (a3, THREAD_FPREG + 36)
	ldw      r8, (a3, THREAD_FPREG + 40)
	ldw      r9, (a3, THREAD_FPREG + 44)
	fmtvrl   vr4, r6
	fmtvrh   vr4, r7
	fmtvrl   vr5, r8
	fmtvrh   vr5, r9
	ldw      r6, (a3, THREAD_FPREG + 48)
	ldw      r7, (a3, THREAD_FPREG + 52)
	ldw      r8, (a3, THREAD_FPREG + 56)
	ldw      r9, (a3, THREAD_FPREG + 60)
	fmtvrl   vr6, r6
	fmtvrh   vr6, r7
	fmtvrl   vr7, r8
	fmtvrh   vr7, r9
	ldw      r6, (a3, THREAD_FPREG + 64)
	ldw      r7, (a3, THREAD_FPREG + 68)
	ldw      r8, (a3, THREAD_FPREG + 72)
	ldw      r9, (a3, THREAD_FPREG + 76)
	fmtvrl   vr8, r6
	fmtvrh   vr8, r7
	fmtvrl   vr9, r8
	fmtvrh   vr9, r9
	ldw      r6, (a3, THREAD_FPREG + 80)
	ldw      r7, (a3, THREAD_FPREG + 84)
	ldw      r8, (a3, THREAD_FPREG + 88)
	ldw      r9, (a3, THREAD_FPREG + 92)
	fmtvrl   vr10, r6
	fmtvrh   vr10, r7
	fmtvrl   vr11, r8
	fmtvrh   vr11, r9
	ldw      r6, (a3, THREAD_FPREG + 96)
	ldw      r7, (a3, THREAD_FPREG + 100)
	ldw      r8, (a3, THREAD_FPREG + 104)
	ldw      r9, (a3, THREAD_FPREG + 108)
	fmtvrl   vr12, r6
	fmtvrh   vr12, r7
	fmtvrl   vr13, r8
	fmtvrh   vr13, r9
	ldw      r6, (a3, THREAD_FPREG + 112)
	ldw      r7, (a3, THREAD_FPREG + 116)
	ldw      r8, (a3, THREAD_FPREG + 120)
	ldw      r9, (a3, THREAD_FPREG + 124)
	fmtvrl   vr14, r6
	fmtvrh   vr14, r7
	fmtvrl   vr15, r8
	fmtvrh   vr15, r9
.endm

#endif /* __ASSEMBLY__ */

#endif /* __ASM_CSKY_FPU_H */
