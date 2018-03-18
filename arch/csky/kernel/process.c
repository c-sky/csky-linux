// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/module.h>
#include <linux/version.h>
#include <linux/sched.h>
#ifndef COMPAT_KERNEL_4_9
#include <linux/sched/task_stack.h>
#include <linux/sched/debug.h>
#endif
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/uaccess.h>
#include <asm/elf.h>
#include <linux/ptrace.h>

struct cpuinfo_csky cpu_data[NR_CPUS];

asmlinkage void ret_from_fork(void);
asmlinkage void ret_from_kernel_thread(void);

/*
 * Some archs flush debug and FPU info here
 */
void flush_thread(void){}

/*
 * Return saved PC from a blocked thread
 */
unsigned long thread_saved_pc(struct task_struct *tsk)
{
	struct switch_stack *sw = (struct switch_stack *)tsk->thread.ksp;

	return sw->r15;
}

int copy_thread(unsigned long clone_flags,
		unsigned long usp,
		unsigned long kthread_arg,
		struct task_struct *p)
{
	struct switch_stack * childstack;
	unsigned long reg_psr = 0;
	struct pt_regs *childregs = task_pt_regs(p);

	preempt_disable();

	asm volatile("mfcr %0, psr\n":"=r"(reg_psr));

#ifdef CONFIG_CPU_HAS_FPU
	save_fp_to_thread(p->thread.fp, &p->thread.fcr, &p->thread.fsr,
	     &p->thread.fesr);
#endif
#ifdef CONFIG_CPU_HAS_HILO
	asm volatile(
		"mfhi	%0 \n"
		"mflo	%1 \n"
		:"=r"(p->thread.hi),"=r"(p->thread.lo));
#endif
	preempt_enable();

	childstack = ((struct switch_stack *) childregs) - 1;
	memset(childstack, 0, sizeof(struct switch_stack));

	/* setup ksp for switch_to !!! */
	p->thread.ksp = (unsigned long)childstack;

	if (unlikely(p->flags & PF_KTHREAD)) {
		memset(childregs, 0, sizeof(struct pt_regs));
		childstack->r15 = (unsigned long) ret_from_kernel_thread;
		childstack->r8 = kthread_arg;
		childstack->r9 = usp;
		childregs->sr = reg_psr;

		return 0;
	} else {
		*childregs = *(current_pt_regs());
		childstack->r15 = (unsigned long) ret_from_fork;
	}

	/* Return 0 for subprocess when return from fork(),vfork(),clone() */
	childregs->a0 = 0;

	if (usp != 0)
		p->thread.usp = usp;
	else
		p->thread.usp = rdusp();

	if (clone_flags & CLONE_SETTLS) {
		task_thread_info(p)->tp_value = (current_pt_regs())->regs[0];
#ifdef __CSKYABIV2__
		childregs->exregs[15] = task_thread_info(p)->tp_value;
#endif
	}

	return 0;
}

/* Fill in the fpu structure for a core dump.  */
int dump_fpu (struct pt_regs *regs, struct user_cskyfp_struct *fpu)
{
	memcpy(fpu, &current->thread.fcr, sizeof(*fpu));
	return 1;
}
EXPORT_SYMBOL(dump_fpu);

int dump_task_regs(struct task_struct *tsk, elf_gregset_t *pr_regs)
{
	struct pt_regs *regs = (struct pt_regs *)(tsk->thread.esp0);

	/* NOTE: usp is error value. */
	ELF_CORE_COPY_REGS ((*pr_regs), regs)

	/* Now fix usp in pr_regs, usp is in pr_regs[2] */
#if defined(__CSKYABIV2__)
	(*pr_regs)[16] = tsk->thread.usp;
#else
	(*pr_regs)[2] = tsk->thread.usp;
#endif

	return 1;
}

unsigned long get_wchan(struct task_struct *p)
{
	unsigned long esp, pc;
	unsigned long stack_page;
	int count = 0;
	if (!p || p == current || p->state == TASK_RUNNING)
		return 0;

	stack_page = (unsigned long)p;
	esp = p->thread.esp0;
	do {
		if (esp < stack_page+sizeof(struct task_struct) ||
		    esp >= 8184+stack_page)
			return 0;
		/*FIXME: There's may be error here!*/
		pc = ((unsigned long *)esp)[1];
		/* FIXME: This depends on the order of these functions. */
		if (!in_sched_functions(pc))
			return pc;
		esp = *(unsigned long *) esp;
	} while (count++ < 16);
	return 0;
}

EXPORT_SYMBOL(get_wchan);
