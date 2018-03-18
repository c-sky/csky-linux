// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef _CSKY_PTRACE_H
#define _CSKY_PTRACE_H

#define REGNO_SR   	32
#define REGNO_PC   	33

#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the
   stack during a system call. */
struct  pt_regs {
	unsigned long    pc;
	long             orig_a0;
	unsigned long    sr;
	long             a0;  // ABIV2: r0, ABIV1: r2
	long             a1;  // ABIV2: r1, ABIV1: r3
	long             a2;  // ABIV2: r2, ABIV1: r4
	long             a3;  // ABIV2: r3, ABIV1: r5
	// ABIV2: r4 ~ r13,  ABIV1: r6 ~ r14, r1.
	long             regs[10];
	long             r15;
#if defined(__CSKYABIV2__)
	// r16~r31;
	long             exregs[16];
	long             rhi;
	long             rlo;
#endif
};

/*
 * Switch stack for switch_to after push pt_regs.
 *
 * ABI_CSKYV2: r4 ~ r11, r15 ~ r17, r26 ~ r30;
 * ABI_CSKYV1: r8 ~ r14, r15;
 */
struct  switch_stack {
#if defined(__CSKYABIV2__)
	unsigned long   r4;
        unsigned long   r5;
        unsigned long   r6;
        unsigned long   r7;
	unsigned long   r8;
        unsigned long   r9;
        unsigned long   r10;
        unsigned long   r11;
#else
	unsigned long   r8;
        unsigned long   r9;
        unsigned long   r10;
        unsigned long   r11;
        unsigned long   r12;
        unsigned long   r13;
        unsigned long   r14;
#endif
        unsigned long   r15;
#if defined(__CSKYABIV2__)
        unsigned long   r16;
        unsigned long   r17;
        unsigned long   r26;
        unsigned long   r27;
        unsigned long   r28;
        unsigned long   r29;
        unsigned long   r30;
#endif
};

#define PTRACE_GETREGS            12
#define PTRACE_SETREGS            13
#define PTRACE_GETFPREGS          14
#define PTRACE_SETFPREGS          15
#define PTRACE_GET_THREAD_AREA    25

#define CSKY_GREG_NUM             35
#define CSKY_FREG_NUM_HI          72
#define CSKY_FREG_NUM_LO          40
#define CSKY_FCR_NUM              74

#ifdef __KERNEL__

#define PS_S            0x80000000              /* Supervisor Mode */

#define arch_has_single_step() (1)
#define current_pt_regs() \
	(struct pt_regs *)((char *)current_thread_info() + THREAD_SIZE) - 1
#define current_user_stack_pointer() rdusp()

#define user_mode(regs) (!((regs)->sr & PS_S))
#define instruction_pointer(regs) ((regs)->pc)
#define profile_pc(regs) instruction_pointer(regs)

void show_regs(struct pt_regs *);

#endif /* __KERNEL__ */
#endif /* __ASSEMBLY__ */
#endif /* _CSKY_PTRACE_H */
