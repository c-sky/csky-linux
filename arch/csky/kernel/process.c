#include <linux/sched.h>
#include <linux/module.h>

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

void show_regs(struct pt_regs * regs)
{
	printk("\n");
	printk("PC: %08lx  Status: %04lx  orig_a0: %08lx  %s\n",
                regs->pc, regs->sr, regs->orig_a0, print_tainted());

#if defined(__CSKYABIV1__)
	printk("r2: %08lx   r3: %08lx   r4: %08lx   r5: %08lx \n",
		regs->a0, regs->a1, regs->a2, regs->a3);
	printk("r6: %08lx   r7: %08lx   r8: %08lx   r9: %08lx \n",
		regs->regs[0], regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("r10: %08lx  r11: %08lx  r12: %08lx  r13: %08lx \n",
		regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("r14: %08lx  r1: %08lx  r15: %08lx\n \n",
		regs->regs[8], regs->regs[9], regs->r15);
#endif

#if defined(__CSKYABIV2__)
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

int copy_thread(unsigned long clone_flags,
		unsigned long usp,
		unsigned long kthread_arg,
		struct task_struct *p)
{
	struct switch_stack * childstack;

	unsigned long reg_psr = 0;

	struct pt_regs *childregs = task_pt_regs(p);

	__asm__ __volatile__("mfcr   %0, psr\n\t"
			             :"+r"(reg_psr) :);


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

