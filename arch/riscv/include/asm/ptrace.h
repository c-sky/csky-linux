/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#ifndef _ASM_RISCV_PTRACE_H
#define _ASM_RISCV_PTRACE_H

#include <uapi/asm/ptrace.h>
#include <asm/csr.h>
#include <linux/compiler.h>

#ifndef __ASSEMBLY__

struct pt_regs {
	xlen_t epc;
	xlen_t ra;
	xlen_t sp;
	xlen_t gp;
	xlen_t tp;
	xlen_t t0;
	xlen_t t1;
	xlen_t t2;
	xlen_t s0;
	xlen_t s1;
	xlen_t a0;
	xlen_t a1;
	xlen_t a2;
	xlen_t a3;
	xlen_t a4;
	xlen_t a5;
	xlen_t a6;
	xlen_t a7;
	xlen_t s2;
	xlen_t s3;
	xlen_t s4;
	xlen_t s5;
	xlen_t s6;
	xlen_t s7;
	xlen_t s8;
	xlen_t s9;
	xlen_t s10;
	xlen_t s11;
	xlen_t t3;
	xlen_t t4;
	xlen_t t5;
	xlen_t t6;
	/* Supervisor/Machine CSRs */
	xlen_t status;
	xlen_t badaddr;
	xlen_t cause;
	/* a0 value before the syscall */
	xlen_t orig_a0;
};

#define PTRACE_SYSEMU			0x1f
#define PTRACE_SYSEMU_SINGLESTEP	0x20

#if __riscv_xlen == 64
#define REG_FMT "%016llx"
#else
#define REG_FMT "%08x"
#endif

#define user_mode(regs) (((regs)->status & SR_PP) == 0)

#define MAX_REG_OFFSET offsetof(struct pt_regs, orig_a0)

/* Helpers for working with the instruction pointer */
static inline unsigned long instruction_pointer(struct pt_regs *regs)
{
	return (unsigned long)regs->epc;
}
static inline void instruction_pointer_set(struct pt_regs *regs,
					   unsigned long val)
{
	regs->epc = (xlen_t)val;
}

#define profile_pc(regs) instruction_pointer(regs)

/* Helpers for working with the user stack pointer */
static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
	return (unsigned long)regs->sp;
}
static inline void user_stack_pointer_set(struct pt_regs *regs,
					  unsigned long val)
{
	regs->sp = (xlen_t)val;
}

/* Valid only for Kernel mode traps. */
static inline unsigned long kernel_stack_pointer(struct pt_regs *regs)
{
	return (unsigned long)regs->sp;
}

/* Helpers for working with the frame pointer */
static inline unsigned long frame_pointer(struct pt_regs *regs)
{
	return (unsigned long)regs->s0;
}
static inline void frame_pointer_set(struct pt_regs *regs,
				     unsigned long val)
{
	regs->s0 = (xlen_t)val;
}

static inline unsigned long regs_return_value(struct pt_regs *regs)
{
	return (unsigned long)regs->a0;
}

static inline void regs_set_return_value(struct pt_regs *regs,
					 unsigned long val)
{
	regs->a0 = (xlen_t)val;
}

extern int regs_query_register_offset(const char *name);
extern unsigned long regs_get_kernel_stack_nth(struct pt_regs *regs,
					       unsigned int n);

void prepare_ftrace_return(unsigned long *parent, unsigned long self_addr,
			   unsigned long frame_pointer);

/**
 * regs_get_register() - get register value from its offset
 * @regs:	pt_regs from which register value is gotten
 * @offset:	offset of the register.
 *
 * regs_get_register returns the value of a register whose offset from @regs.
 * The @offset is the offset of the register in struct pt_regs.
 * If @offset is bigger than MAX_REG_OFFSET, this returns 0.
 */
static inline unsigned long regs_get_register(struct pt_regs *regs,
					      unsigned int offset)
{
	if (unlikely(offset > MAX_REG_OFFSET))
		return 0;

	return *(unsigned long *)((unsigned long)regs + offset);
}

/**
 * regs_get_kernel_argument() - get Nth function argument in kernel
 * @regs:       pt_regs of that context
 * @n:          function argument number (start from 0)
 *
 * regs_get_argument() returns @n th argument of the function call.
 *
 * Note you can get the parameter correctly if the function has no
 * more than eight arguments.
 */
static inline unsigned long regs_get_kernel_argument(struct pt_regs *regs,
						unsigned int n)
{
	static const int nr_reg_arguments = 8;
	static const unsigned int argument_offs[] = {
		offsetof(struct pt_regs, a0),
		offsetof(struct pt_regs, a1),
		offsetof(struct pt_regs, a2),
		offsetof(struct pt_regs, a3),
		offsetof(struct pt_regs, a4),
		offsetof(struct pt_regs, a5),
		offsetof(struct pt_regs, a6),
		offsetof(struct pt_regs, a7),
	};

	if (n < nr_reg_arguments)
		return regs_get_register(regs, argument_offs[n]);
	return 0;
}

static inline int regs_irqs_disabled(struct pt_regs *regs)
{
	return !(regs->status & SR_PIE);
}

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_PTRACE_H */
