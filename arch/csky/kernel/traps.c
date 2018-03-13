#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/user.h>
#include <linux/string.h>
#include <linux/linkage.h>
#include <linux/init.h>
#include <linux/ptrace.h>
#include <linux/kallsyms.h>
#include <linux/rtc.h>

#include <asm/setup.h>
#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/pgalloc.h>
#include <asm/siginfo.h>

#include <asm/mmu_context.h>

#ifdef CONFIG_CPU_HAS_FPU
#include <abi/fpu.h>
#endif

/* Defined in entry.S */
asmlinkage void csky_trap(void);

asmlinkage void csky_systemcall(void);
asmlinkage void csky_cmpxchg(void);
asmlinkage void csky_get_tls(void);
asmlinkage void csky_irq(void);

asmlinkage void csky_tlbinvalidl(void);
asmlinkage void csky_tlbinvalids(void);
asmlinkage void csky_tlbmodified(void);

void __init pre_trap_init(void)
{
	int i;

	__asm__ __volatile__(
		"mtcr %0, vbr\n"
		::"r"(vec_base));

	for(i=1;i<128;i++) VEC_INIT(i, csky_trap);
}

void __init trap_init (void)
{
	int i;

	/* setup irq */
	for(i=32;i<128;i++)
		  VEC_INIT(i, csky_irq);
	VEC_INIT(VEC_AUTOVEC, csky_irq);

	/* setup trap0 trap2 trap3 */
	VEC_INIT(VEC_TRAP0, csky_systemcall);
	VEC_INIT(VEC_TRAP2, csky_cmpxchg);
	VEC_INIT(VEC_TRAP3, csky_get_tls);

	/* setup MMU TLB exception */
	VEC_INIT(VEC_TLBINVALIDL, csky_tlbinvalidl);
	VEC_INIT(VEC_TLBINVALIDS, csky_tlbinvalids);
	VEC_INIT(VEC_TLBMODIFIED, csky_tlbmodified);


#ifdef CONFIG_CPU_HAS_FPU
	init_fpu();
#endif
}

void die_if_kernel (char *str, struct pt_regs *regs, int nr)
{
	if (user_mode(regs)) return;

	console_verbose();
	pr_err("%s: %08x\n",str,nr);
        show_regs(regs);
        add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	do_exit(SIGSEGV);
}

void buserr(struct pt_regs *regs)
{
	siginfo_t info;

	die_if_kernel("Kernel mode BUS error", regs, 0);

	pr_err("User mode Bus Error\n");
	show_regs(regs);

	current->thread.esp0 = (unsigned long) regs;
	info.si_signo = SIGSEGV;
	info.si_errno = 0;
	force_sig_info(SIGSEGV, &info, current);
}

int kstack_depth_to_print = 48;

void show_trace(unsigned long *stack)
{
	unsigned long *endstack;
	unsigned long addr;
	int i;

	pr_info("Call Trace:");
	addr = (unsigned long)stack + THREAD_SIZE - 1;
	endstack = (unsigned long *)(addr & -THREAD_SIZE);
	i = 0;
	while (stack + 1 <= endstack) {
		addr = *stack++;
		/*
		 * If the address is either in the text segment of the
		 * kernel, or in the region which contains vmalloc'ed
		 * memory, it *may* be the address of a calling
		 * routine; if so, print it so that someone tracing
		 * down the cause of the crash will be able to figure
		 * out the call path that was taken.
		 */
		if (__kernel_text_address(addr)) {
#ifndef CONFIG_KALLSYMS
			if (i % 5 == 0)
				pr_cont("\n       ");
#endif
			pr_cont(" [<%08lx>] %pS\n", addr, (void *)addr);
			i++;
		}
	}
	pr_cont("\n");
}

void show_stack(struct task_struct *task, unsigned long *stack)
{
	unsigned long *p;
	unsigned long *endstack;
	int i;

	if (!stack) {
		if (task)
			stack = (unsigned long *)task->thread.esp0;
		else
			stack = (unsigned long *)&stack;
	}
	endstack = (unsigned long *)(((unsigned long)stack + THREAD_SIZE - 1) & -THREAD_SIZE);

	pr_info("Stack from %08lx:", (unsigned long)stack);
	p = stack;
	for (i = 0; i < kstack_depth_to_print; i++) {
		if (p + 1 > endstack)
			break;
		if (i % 8 == 0)
			pr_cont("\n       ");
		pr_cont(" %08lx", *p++);
	}
	pr_cont("\n");
	show_trace(stack);
}

extern void alignment_c(struct pt_regs *regs);
asmlinkage void trap_c(struct pt_regs *regs)
{
	int sig;
	unsigned long vector;
	siginfo_t info;

	asm volatile("mfcr %0, psr":"=r"(vector));

	vector = (vector >> 16) & 0xff;

	switch (vector) {
		case VEC_ZERODIV:
			sig = SIGFPE;
			break;
		/* ptrace */
		case VEC_TRACE:
			info.si_code = TRAP_TRACE;
			sig = SIGTRAP;
			break;

		/* gdbserver  breakpoint */
		case VEC_TRAP1:
		/* jtagserver breakpoint */
		case VEC_BREAKPOINT:
			info.si_code = TRAP_BRKPT;
			sig = SIGTRAP;
			break;
		case VEC_ACCESS:
			return buserr(regs);
		case VEC_ALIGN:
			return alignment_c(regs);
#ifdef CONFIG_CPU_HAS_FPU
		case VEC_FPE:
			return fpu_fpe(regs);
		case VEC_PRIV:
			if(fpu_libc_helper(regs)) return;
#endif
		default:
			sig = SIGILL;
			break;
	}
	send_sig(sig, current, 0);
}

asmlinkage void set_esp0 (unsigned long ssp)
{
	current->thread.esp0 = ssp;
}

