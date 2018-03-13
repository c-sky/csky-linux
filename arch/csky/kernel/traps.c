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

void show_registers(struct pt_regs *fp);

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
	printk("%s: %08x\n",str,nr);
        show_registers(regs);
        add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	do_exit(SIGSEGV);
}

void buserr(struct pt_regs *regs)
{
	siginfo_t info;

	die_if_kernel("Kernel mode BUS error", regs, 0);

	printk("User mode Bus Error\n");
	show_registers(regs);

	current->thread.esp0 = (unsigned long) regs;
	info.si_signo = SIGSEGV;
	info.si_errno = 0;
	force_sig_info(SIGSEGV, &info, current);
}

int kstack_depth_to_print = 48;

/* MODULE_RANGE is a guess of how much space is likely to be
   vmalloced.  */
#define MODULE_RANGE (8*1024*1024)

void show_trace(unsigned long *stack)
{
        unsigned long *endstack;
        unsigned long addr;
        int i;

        printk("Call Trace:");
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
                                printk("\n       ");
#endif
                        printk(" [<%08lx>] %pS\n", addr, (void *)addr);
                        i++;
                }
        }
        printk("\n");
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

        printk("Stack from %08lx:", (unsigned long)stack);
        p = stack;
        for (i = 0; i < kstack_depth_to_print; i++) {
                if (p + 1 > endstack)
                        break;
                if (i % 8 == 0)
                        printk("\n       ");
                printk(" %08lx", *p++);
        }
        printk("\n");
        show_trace(stack);
}

/*
 * The architecture-independent backtrace generator
 */
void dump_stack(void)
{
	unsigned long stack;

	show_trace(&stack);
}
EXPORT_SYMBOL(dump_stack);

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

void show_trace_task(struct task_struct *tsk)
{
	/* DAVIDM: we can do better, need a proper stack dump */
	printk("STACK ksp=0x%lx, usp=0x%lx\n", tsk->thread.ksp, tsk->thread.usp);
}

/*
 *      Generic dumping code. Used for panic and debug.
 */
void show_registers(struct pt_regs *fp)
{
	unsigned long   *sp;
	unsigned char   *tp;
	int             i;

	printk("\nCURRENT PROCESS:\n\n");
	printk("COMM=%s PID=%d\n", current->comm, current->pid);

	if (current->mm) {
		printk("TEXT=%08x-%08x DATA=%08x-%08x BSS=%08x-%08x\n",
		        (int) current->mm->start_code,
		        (int) current->mm->end_code,
		        (int) current->mm->start_data,
		        (int) current->mm->end_data,
		        (int) current->mm->end_data,
		        (int) current->mm->brk);
		printk("USER-STACK=%08x  KERNEL-STACK=%08x\n\n",
		        (int) current->mm->start_stack,
		        (int) (((unsigned long) current) + 2 * PAGE_SIZE));
	}

	printk("PC: 0x%08lx\n", (long)fp->pc);
	printk("orig_a0: 0x%08lx\n", fp->orig_a0);
	printk("PSR: 0x%08lx\n", (long)fp->sr);
	/*
	 * syscallr1->orig_a0
	 * please refer asm/ptrace.h
	 */
#if defined(__CSKYABIV2__)
	printk("r0: 0x%08lx  r1: 0x%08lx  r2: 0x%08lx  r3: 0x%08lx\n",
		fp->a0, fp->a1, fp->a2, fp->a3);
	printk("r4: 0x%08lx  r5: 0x%08lx    r6: 0x%08lx    r7: 0x%08lx\n",
		fp->regs[0], fp->regs[1], fp->regs[2], fp->regs[3]);
	printk("r8: 0x%08lx  r9: 0x%08lx   r10: 0x%08lx   r11: 0x%08lx\n",
		fp->regs[4], fp->regs[5], fp->regs[6], fp->regs[7]);
	printk("r12 0x%08lx  r13: 0x%08lx   r15: 0x%08lx\n",
		fp->regs[8], fp->regs[9], fp->r15);
#else
	printk("r2: 0x%08lx   r3: 0x%08lx   r4: 0x%08lx   r5: 0x%08lx\n",
		fp->a0, fp->a1, fp->a2, fp->a3);
	printk("r6: 0x%08lx   r7: 0x%08lx   r8: 0x%08lx   r9: 0x%08lx\n",
		fp->regs[0], fp->regs[1], fp->regs[2], fp->regs[3]);
	printk("r10: 0x%08lx   r11: 0x%08lx   r12: 0x%08lx   r13: 0x%08lx\n",
		fp->regs[4], fp->regs[5], fp->regs[6], fp->regs[7]);
	printk("r14 0x%08lx   r1: 0x%08lx   r15: 0x%08lx\n",
		fp->regs[8], fp->regs[9], fp->r15);
#endif
#if defined(__CSKYABIV2__)
	printk("r16:0x%08lx   r17: 0x%08lx   r18: 0x%08lx    r19: 0x%08lx\n",
                fp->exregs[0], fp->exregs[1], fp->exregs[2], fp->exregs[3]);
	printk("r20 0x%08lx   r21: 0x%08lx   r22: 0x%08lx    r23: 0x%08lx\n",
		fp->exregs[4], fp->exregs[5], fp->exregs[6], fp->exregs[7]);
	printk("r24 0x%08lx   r25: 0x%08lx   r26: 0x%08lx    r27: 0x%08lx\n",
		fp->exregs[8], fp->exregs[9], fp->exregs[10], fp->exregs[11]);
	printk("r28 0x%08lx   r29: 0x%08lx   r30: 0x%08lx    r31: 0x%08lx\n",
		fp->exregs[12], fp->exregs[13], fp->exregs[14], fp->exregs[15]);
	printk("hi 0x%08lx     lo: 0x%08lx \n",
                fp->rhi, fp->rlo);
#endif

	printk("\nCODE:");
	tp = ((unsigned char *) fp->pc) - 0x20;
	tp += ((int)tp % 4) ? 2 : 0;
	for (sp = (unsigned long *) tp, i = 0; (i < 0x40);  i += 4) {
		if ((i % 0x10) == 0)
		        printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n");

	printk("\nKERNEL STACK:");
	tp = ((unsigned char *) fp) - 0x40;
	for (sp = (unsigned long *) tp, i = 0; (i < 0xc0); i += 4) {
		if ((i % 0x10) == 0)
		        printk("\n%08x: ", (int) (tp + i));
		printk("%08x ", (int) *sp++);
	}
	printk("\n");

	return;
}

