#include <asm/system.h>
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
#include <asm/fpu.h>
#include <asm/uaccess.h>
#include <asm/traps.h>
#include <asm/pgalloc.h>
#include <asm/machdep.h>
#include <asm/siginfo.h>
#include <asm/misc_csky.h>

#include <asm/mmu_context.h>

extern void csky_tlb_init(void);
void show_registers(struct pt_regs *fp);
/* assembler routines */
asmlinkage void buserr(void);
asmlinkage void alignment(void);
asmlinkage void trap(void);
asmlinkage void trap1(void);
asmlinkage void trap2(void);
asmlinkage void trap3(void);
asmlinkage void system_call(void);
asmlinkage void inthandler(void);
asmlinkage void autohandler(void);
asmlinkage void fastautohandler(void);

asmlinkage void handle_tlbinvalidl(void);
asmlinkage void handle_tlbinvalids(void);
asmlinkage void handle_tlbmodified(void);
asmlinkage void handle_tlbmiss(void);
asmlinkage void handle_fpe(void);
asmlinkage void handle_illegal(void);

void __init per_cpu_trap_init(void)
{
	unsigned int cpu = smp_processor_id();

	csky_tlb_init();
	cpu_data[cpu].asid_cache = ASID_FIRST_VERSION;
	current_cpu_data.asid_cache = ASID_FIRST_VERSION;

	TLBMISS_HANDLER_SETUP_PGD(swapper_pg_dir);

	atomic_inc(&init_mm.mm_count);
	current->active_mm = &init_mm;
	BUG_ON(current->mm);
}

extern e_vector vec_base;

void __init trap_init (void)
{
	int i;
	e_vector *_ramvec = &vec_base;

	__asm__ __volatile__(
		"mtcr %0, vbr\n"
		::"b"(_ramvec));

	per_cpu_trap_init();

	  /*
	 *      There is a common trap handler and common interrupt
	 *      handler that handle almost every vector. We treat
	 *      the system call and bus error special, they get their
	 *      own first level handlers.
	 */
	for(i = 1; (i <= 31); i++)
	{
		_ramvec[i] = trap;
	}
	for(; (i < 128); i++)
	{
		_ramvec[i] = inthandler;
	}

	_ramvec[VEC_ACCESS] = buserr;
	_ramvec[VEC_ALIGN] = alignment;
	_ramvec[VEC_TRAP0] = system_call;
	_ramvec[VEC_TRAP1] = trap1;
	_ramvec[VEC_TRAP2] = trap2;
	_ramvec[VEC_TRAP3] = trap3;
	_ramvec[VEC_AUTOVEC] = autohandler;

	_ramvec[VEC_TLBINVALIDL] = handle_tlbinvalidl;
	_ramvec[VEC_TLBINVALIDS] = handle_tlbinvalids;
	_ramvec[VEC_TLBMODIFIED] = handle_tlbmodified;
	_ramvec[VEC_TLBMISS] = (void *)((unsigned int)handle_tlbmiss | 0x1);
	_ramvec[VEC_ILLEGAL] = handle_illegal;
	_ramvec[VEC_FPE] = handle_fpe;
}

void die_if_kernel(char *,struct pt_regs *,int);
asmlinkage int do_page_fault(struct pt_regs *regs, unsigned long address,
                             unsigned long error_code);

asmlinkage void trap_c(int vector, struct frame *fp);
asmlinkage void handle_fpe_c(unsigned long fesr, struct pt_regs * regs);
asmlinkage void handle_illegal_c(struct pt_regs * regs);

asmlinkage void buserr_c(struct frame *fp)
{
	printk("%s(%d): Bus Error Trap\n", __FILE__, __LINE__);
	show_registers((struct pt_regs *) fp);
	/* Only set esp0 if coming from user mode */
	if (user_mode(&fp->ptregs)){
		current->thread.esp0 = (unsigned long) fp;
    } else{
		die_if_kernel("Kernel mode BUS error", (struct pt_regs *)fp, 0);
    }
	force_sig(SIGSEGV, current);
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

void bad_super_trap (int vector, struct frame *fp)
{
	console_verbose();

	printk("Kernel: Bad trap from supervisor state, vector = %d\n", vector);
	printk("Current process id is %d\n", current->pid);
	show_registers((struct pt_regs *)fp);
	panic("Trap from supervisor state\n");
}

/* use as fpc control reg read/write in glibc. */
int hand_fpcr_rdwr(struct pt_regs * regs)
{
	mm_segment_t fs;
	unsigned long instrptr, regx = 0;
	unsigned int fault;
#if   defined(CONFIG_CPU_CSKYV1)
	unsigned long index_regx = 0, index_fpregx = 0;
	u16 tinstr = 0;

	instrptr = instruction_pointer(regs);

	fs = get_fs();
	set_fs(KERNEL_DS);
	fault = __get_user(tinstr, (u16 *)(instrptr & ~1));
	set_fs(fs);
	if (fault) {
		goto bad_or_fault;
	}

	index_regx = tinstr & 0x7;
	index_fpregx = ((tinstr >> 3) & 0x17);
	tinstr = tinstr >> 8;
	if(tinstr == 0x6f)
	{
		regx = read_pt_regs(index_regx, regs);
		if(index_fpregx == 1) {
			write_fpcr(regx);
		}
		else if(index_fpregx == 4) {
			write_fpesr(regx);
		} else
			goto bad_or_fault;

		regs->pc +=2;
		return 1;
	}
	else if(tinstr == 0x6b)
	{
		if(index_fpregx == 1) {
			regx = read_fpcr();
		}
		else if(index_fpregx == 2) { /* need test fpu is busy */
			regx = read_fpsr();
		}
		else if(index_fpregx == 4) {
			regx = read_fpesr();
		} else
			goto bad_or_fault;

		write_pt_regs(regx, index_regx, regs);
		regs->pc +=2;
		return 1;
	}
#elif defined(CONFIG_CPU_CSKYV2)
	u16 instr_hi, instr_low;
	unsigned long index_regx = 0, index_fpregx_prev = 0, index_fpregx_next = 0;
	unsigned long tinstr = 0;

	instrptr = instruction_pointer(regs);

	/* CSKYV2's 32 bit instruction may not align 4 words */
	fs = get_fs();
	set_fs(KERNEL_DS);
	fault = __get_user(instr_low, (u16 *)(instrptr & ~1));
	set_fs(fs);
	if (fault) {
		goto bad_or_fault;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	fault = __get_user(instr_hi, (u16 *)((instrptr + 2) & ~1));
	set_fs(fs);
	if (fault) {
		goto bad_or_fault;
	}

	tinstr = instr_hi | ((unsigned long)instr_low << 16);

	index_fpregx_next = ((tinstr >> 21) & 0x1F);

	/* just want to handle instruction which opration cr<1, 2> or cr<2, 2> */
	if(index_fpregx_next != 2){
		goto bad_or_fault;
	}

/*
 * define four macro to distinguish the instruction is mfcr or mtcr.
 */
#define MTCR_MASK 0xFC00FFE0
#define MFCR_MASK 0xFC00FFE0
#define MTCR_DISTI 0xC0006420
#define MFCR_DISTI 0xC0006020

	if ((tinstr & MTCR_MASK) == MTCR_DISTI)
	{
		index_regx = (tinstr >> 16) & 0x1F;
		index_fpregx_prev = tinstr & 0x1F;

		regx = read_pt_regs(index_regx, regs);

		if(index_fpregx_prev == 1) {
			write_fpcr(regx);
		} else if (index_fpregx_prev == 2) {
			write_fpesr(regx);
		} else {
			goto bad_or_fault;
		}

		regs->pc +=4;
		return 1;
	} else if ((tinstr & MFCR_MASK) == MFCR_DISTI) {
		index_regx = tinstr & 0x1F;
		index_fpregx_prev = ((tinstr >> 16) & 0x1F);

		if (index_fpregx_prev == 1) {
			regx = read_fpcr();
		} else if (index_fpregx_prev == 2) {
			regx = read_fpesr();
		} else {
			goto bad_or_fault;
		}

		write_pt_regs(regx, index_regx, regs);

		regs->pc +=4;
		return 1;
	}

#else
#error must choose CSKYV1 or CSKYV2
#endif /* define CONFIG_CPU_CSKYV2 */
bad_or_fault:
	return 0;
}

asmlinkage void trap_c(int vector, struct frame *fp)
{
	int sig;
	siginfo_t info;

	/* send the appropriate signal to the user program */
	switch (vector) {
		case VEC_ZERODIV:
			sig = SIGFPE;
			break;

		case VEC_PRIV:  /* some reg fcr of ck610 only R/W in super mode. */
			sig = SIGILL;
			if(hand_fpcr_rdwr((struct pt_regs *)fp))
				return;
			break;

		case VEC_TRACE:  /* ptrace single step */
			info.si_code = TRAP_TRACE;
			sig = SIGTRAP;
			break;

		    /* fp->ptregs.sr &= ~PS_T */
		case VEC_TRAP1:    /* gdb server breakpoint */
		case VEC_BREAKPOINT: /* breakpoint */
			info.si_code = TRAP_BRKPT;
			sig = SIGTRAP;
			break;

		default:
			sig = SIGILL;
			break;
	}
	send_sig(sig, current, 0);
}

asmlinkage void handle_fpe_c(unsigned long fesr, struct pt_regs * regs)
{
	int sig;
	siginfo_t info;

	if(fesr & FPE_ILLE){
		info.si_code = ILL_ILLOPC;
		sig = SIGILL;
	}
	else if(fesr & FPE_IDC){
		info.si_code = ILL_ILLOPN;
		sig = SIGILL;
	}
	else if(fesr & FPE_FEC){
		sig = SIGFPE;
		if(fesr & FPE_IOC){
			info.si_code = FPE_FLTINV;
		}
		else if(fesr & FPE_DZC){
			info.si_code = FPE_FLTDIV;
		}
		else if(fesr & FPE_UFC){
			info.si_code = FPE_FLTUND;
		}
		else if(fesr & FPE_OFC){
			info.si_code = FPE_FLTOVF;
		}
		else if(fesr & FPE_IXC){
			info.si_code = FPE_FLTRES;
		}
		else {
		info.si_code = __SI_FAULT;
		}
	}
	else {
		info.si_code = __SI_FAULT;
		sig = SIGFPE;
	}
	info.si_signo = SIGFPE;
	info.si_errno = 0;
	info.si_addr = (void *)regs->pc;
	force_sig_info(sig, &info, current);
}


asmlinkage void handle_illegal_c(struct pt_regs * regs)
{
	int sig;
	sig = SIGILL;
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

void die_if_kernel (char *str, struct pt_regs *fp, int nr)
{
	if (!(fp->sr & PS_S))
		return;

	console_verbose();
	printk("%s: %08x\n",str,nr);
        show_registers(fp);
        add_taint(TAINT_DIE, LOCKDEP_NOW_UNRELIABLE);
	do_exit(SIGSEGV);
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
#if defined(CONFIG_CPU_CSKYV2)
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

