/*
 * linux/arch/csky/kernel/process.c
 * This file handles the architecture-dependent parts of process handling..
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Hu junshan<junshan_hu@c-sky.com>
 *
 */

#include <asm/system.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/smp.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/ptrace.h>
#include <linux/user.h>
#include <linux/init_task.h>
#include <linux/reboot.h>
#include <linux/mqueue.h>
#include <linux/rtc.h>

#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/machdep.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/user.h>
#include <asm/fpu.h>
#include <asm/regdef.h>

struct cpuinfo_csky cpu_data[NR_CPUS];

asmlinkage void ret_from_fork(void);
asmlinkage void ret_from_kernel_thread(void);

/*
 * Return saved PC from a blocked thread
 */
unsigned long thread_saved_pc(struct task_struct *tsk)
{
	struct switch_stack *sw = (struct switch_stack *)tsk->thread.ksp;

	return sw->r15;
}

void machine_restart(char * __unused)
{
	if (mach_reset)
		mach_reset();
	for (;;);
}

void machine_halt(void)
{
	if (mach_halt)
		mach_halt();
	for (;;);
}

void machine_power_off(void)
{
	if (mach_power_off)
		mach_power_off();
	for (;;);
}

void (*pm_power_off)(void) = machine_power_off;
EXPORT_SYMBOL(pm_power_off);

void show_regs(struct pt_regs * regs)
{
	printk("\n");
	printk("PC: %08lx  Status: %04lx  orig_a0: %08lx  %s\n",
                regs->pc, regs->sr, regs->orig_a0, print_tainted());
#if defined(__CSKYABIV2__)
	printk(" r0: %08lx  r1: %08lx  r2: %08lx   r3: %08lx \n",
		regs->a0, regs->a1, regs->a2, regs->a3);
	printk("r4: %08lx  r5: %08lx   r6: %08lx   r7: %08lx \n",
		regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("r8: %08lx  r9: %08lx  r10: %08lx  r11: %08lx \n",
		regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("r12: %08lx  r13: %08lx  r15: %08lx\n \n",
		regs->regs[8], regs->regs[9], regs->r15);
#else
	printk("r2: %08lx   r3: %08lx   r4: %08lx   r5: %08lx \n",
		regs->a0, regs->a1, regs->a2, regs->a3);
	printk("r6: %08lx   r7: %08lx   r8: %08lx   r9: %08lx \n",
		regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("r10: %08lx  r11: %08lx  r12: %08lx  r13: %08lx \n",
		regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("r14: %08lx  r1: %08lx  r15: %08lx\n \n",
		regs->regs[8], regs->regs[9], regs->r15);
#endif
#if defined(CONFIG_CPU_CSKYV2)
        printk("r16:0x%08lx    r17: 0x%08lx    r18: 0x%08lx    r19: 0x%08lx\n",
                regs->exregs[0], regs->exregs[1], regs->exregs[2], regs->exregs[3]);
        printk("r20 0x%08lx    r21: 0x%08lx    r22: 0x%08lx    r23: 0x%08lx\n",
                regs->exregs[4], regs->exregs[5], regs->exregs[6], regs->exregs[7]);
        printk("r24 0x%08lx    r25: 0x%08lx    r26: 0x%08lx    r27: 0x%08lx\n",
                regs->exregs[8], regs->exregs[9], regs->exregs[10], regs->exregs[11]);
        printk("r28 0x%08lx    r29: 0x%08lx    r30: 0x%08lx    r31: 0x%08lx\n",
                regs->exregs[12], regs->exregs[13], regs->exregs[14], regs->exregs[15]);
        printk("hi 0x%08lx     lo: 0x%08lx \n",
                regs->rhi, regs->rlo);
#endif

	if (!(regs->sr & PS_S))
		printk("USP: %08lx\n", rdusp());
}

/*
 * Some archs flush debug and FPU info here
 */
void flush_thread(void)
{
}

/*
 * "csky_fork()".. By the time we get here, the
 * non-volatile registers have also been saved on the
 * stack. We do some ugly pointer stuff here.. (see
 * also copy_thread)
 */

asmlinkage int csky_fork(struct pt_regs *regs)
{
#ifdef CONFIG_MMU
	return do_fork(SIGCHLD, rdusp(), 0,NULL,NULL);
#else
	/* can not support in nommu mode */
	return(-EINVAL);
#endif

}

asmlinkage int csky_vfork(struct pt_regs *regs)
{
	return do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD, rdusp(), 0,NULL,NULL);
}

asmlinkage int csky_clone(unsigned long clone_flags, unsigned long newsp,
	    int __user *parent_tidptr, int __user *child_tidptr,
	    int tls_val, struct pt_regs *regs)
{
	if (!newsp)
		newsp = rdusp();
	return do_fork(clone_flags, newsp, 0, parent_tidptr, child_tidptr);
}

int copy_thread(unsigned long clone_flags,
		unsigned long usp,
		unsigned long kthread_arg,
		struct task_struct *p)
{
	struct pt_regs * childregs, *regs = current_pt_regs();
	struct switch_stack * childstack;

	unsigned long reg_psr = 0;

	childregs = (struct pt_regs *) (task_stack_page(p) + THREAD_SIZE) - 1;

	childstack = ((struct switch_stack *) childregs) - 1;
	memset(childstack, 0, sizeof(struct switch_stack));

	p->thread.ksp = (unsigned long)childstack;

	if (unlikely(p->flags & PF_KTHREAD)) {
		memset(childregs, 0, sizeof(struct pt_regs));
		childstack->r15 = (unsigned long) ret_from_kernel_thread;
		childstack->r8 = kthread_arg;
		childstack->r9 = usp;
		__asm__ __volatile__("mfcr   %0, psr\n\t"
			             :"+r"(reg_psr) :);
		childregs->sr = reg_psr;

		return 0;
	} else {
		*childregs = *regs;
		childstack->r15 = (unsigned long) ret_from_fork;
	}

	/*Return 0 for subprocess when return from fork(),vfork(),clone()*/
	childregs->a0 = 0;

	p->thread.usp = usp;

	if (clone_flags & CLONE_SETTLS)
		task_thread_info(p)->tp_value = regs->regs[0];

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

/* use to set tls */
asmlinkage int do_set_thread_area(void * addr, struct pt_regs *reg)
{
	struct thread_info *ti = task_thread_info(current);

	ti->tp_value = (unsigned long)addr;

#if defined(CONFIG_CPU_CSKYV2)
	reg->exregs[15] = (long)addr;  // write r31 int pt_regs
#endif

	return 0;
}

/*
 * These bracket the sleeping functions..
 */

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

#ifdef CONFIG_MMU
/*
 * The vectors page is always readable from user space for the
 * atomic helpers and the signal restart code.  Let's declare a mapping
 * for it so it is visible through ptrace and /proc/<pid>/mem.
 */

int vectors_user_mapping(void)
{
	return 0;
}

//const char *arch_vma_name(struct vm_area_struct *vma)
//{
//	return (vma->vm_start == 0xffff0000) ? "[vectors]" : NULL;
//}
#endif

