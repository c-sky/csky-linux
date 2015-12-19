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

/*
 * Return saved PC from a blocked thread
 */
unsigned long thread_saved_pc(struct task_struct *tsk)
{
	struct switch_stack *sw = (struct switch_stack *)tsk->thread.ksp;

	return sw->r15;
}
#if 0
/*
 * The idle loop on an csky 
 */
static void default_idle(void)
{
	__asm__ __volatile__(
//	                     "wait \n\t"
	                     "mov  sp, sp \n\t"
	                     ::);	
}

void (*idle)(void) = default_idle;

/*
 * The idle thread. There's no useful work to be
 * done, so just try to conserve power and have a
 * low exit latency (ie sit in a loop waiting for
 * somebody to say that they'd like to reschedule)
 */
void cpu_idle(void)
{
	/* endless idle loop with no priority at all */
	while (1) {
		tick_nohz_stop_sched_tick(1);
		while (!need_resched())
			idle();

		tick_nohz_restart_sched_tick();
		preempt_enable_no_resched();
		schedule();
		preempt_disable();
	}
}
#endif
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
 * Shuffle the argument into the correct register before calling the
 * thread function.  a1 is the thread argument, a2 is the pointer to
 * the thread function, and a3 points to the exit function.
 */
extern void kernel_thread_helper(void);
asm(    ".section .text\n\t"
"   .align 2\n\t"
"   .type   kernel_thread_helper, @function\n\t"
"kernel_thread_helper:\n\t"
#ifdef CONFIG_TRACE_IRQFLAGS
"   jbsr  trace_hardirqs_on\n\t"
#endif
"   mov a0, a1\n\t"
"   mov lr, a3\n\t"
"   jmp a2\n\t"
"   .size   kernel_thread_helper, . - kernel_thread_helper\n\t"
"   .previous");

/*
 * Create a kernel thread
 */
pid_t __kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)
{
	struct pt_regs regs;
	unsigned long reg_psr = 0;	

	memset(&regs, 0, sizeof(regs));
	
	regs.a1 = (long)arg;
	regs.a2 = (long)fn;
	regs.a3 = (long)do_exit;
	regs.pc = (unsigned long)kernel_thread_helper;
	__asm__ __volatile__("mfcr   %0, psr\n\t"
			             :"+r"(reg_psr) :);
	regs.sr = reg_psr;
	
	return do_fork(flags|CLONE_VM|CLONE_UNTRACED, 0, 0, NULL, NULL);
}
//EXPORT_SYMBOL(kernel_thread);

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

int copy_thread( unsigned long clone_flags,
		unsigned long usp, unsigned long unused,
		struct task_struct * p)
{
	struct thread_info *ti = task_thread_info(p);
	struct pt_regs * childregs, *regs = current_pt_regs();
	struct switch_stack * childstack;

	childregs = (struct pt_regs *) (task_stack_page(p) + THREAD_SIZE) - 1;

	 /*FIXME: There's may not be preempt_disable and preempt_enable!*/
	preempt_disable();
#ifdef CONFIG_CPU_HAS_FPU
	save_fp_to_thread(p->thread.fp, &p->thread.fcr, &p->thread.fsr,
	     &p->thread.fesr);
#endif
#ifdef CONFIG_CPU_PRFL
	__asm__ __volatile__("cprcr  %0, <0, 0x0>\n\r"
								:"=r"(p->thread.hpcr));
	__asm__ __volatile__("cprcr  %0, <0, 0x1>\n\r"
								:"=r"(p->thread.hpspr));
	__asm__ __volatile__("cprcr  %0, <0, 0x2>\n\r"
								:"=r"(p->thread.hpepr));
	__asm__ __volatile__("cprcr  %0, <0, 0x3>\n\r"
								:"=r"(p->thread.hpsir));
	__asm__ __volatile__("cprgr  %0, <0, 0x0>\n\r"
								:"=r"(p->thread.soft[0]));
	__asm__ __volatile__("cprgr  %0, <0, 0x1>\n\r"
							:"=r"(p->thread.soft[1]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2>\n\r"
							:"=r"(p->thread.soft[2]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3>\n\r"
							:"=r"(p->thread.soft[3]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4>\n\r"
							:"=r"(p->thread.soft[4]));
	__asm__ __volatile__("cprgr  %0, <0, 0x5>\n\r"
							:"=r"(p->thread.soft[5]));
	__asm__ __volatile__("cprgr  %0, <0, 0x6>\n\r"
							:"=r"(p->thread.soft[6]));
	__asm__ __volatile__("cprgr  %0, <0, 0x7>\n\r"
							:"=r"(p->thread.soft[7]));
	__asm__ __volatile__("cprgr  %0, <0, 0x8>\n\r"
							:"=r"(p->thread.soft[8]));
	__asm__ __volatile__("cprgr  %0, <0, 0x9>\n\r"
							:"=r"(p->thread.soft[9]));
	__asm__ __volatile__("cprgr  %0, <0, 0xa>\n\r"
							:"=r"(p->thread.soft[10]));
	__asm__ __volatile__("cprgr  %0, <0, 0xb>\n\r"
	                        :"=r"(p->thread.soft[11]));
	__asm__ __volatile__("cprgr  %0, <0, 0xc>\n\r"
	                        :"=r"(p->thread.soft[12]));
	__asm__ __volatile__("cprgr  %0, <0, 0xd>\n\r"
	                        :"=r"(p->thread.soft[13]));
	__asm__ __volatile__("cprgr  %0, <0, 0xe>\n\r"
	                        :"=r"(p->thread.soft[14]));
	__asm__ __volatile__("cprgr  %0, <0, 0xf>\n\r"
	                        :"=r"(p->thread.soft[15]));
	__asm__ __volatile__("cprgr  %0, <0, 0x10>\n\r"
							:"=r"(p->thread.soft[16]));
	__asm__ __volatile__("cprgr  %0, <0, 0x11>\n\r"
	                        :"=r"(p->thread.soft[17]));
	__asm__ __volatile__("cprgr  %0, <0, 0x12>\n\r"
	                        :"=r"(p->thread.soft[18]));
	__asm__ __volatile__("cprgr  %0, <0, 0x13>\n\r"
	                        :"=r"(p->thread.soft[19]));
	__asm__ __volatile__("cprgr  %0, <0, 0x14>\n\r"
	                        :"=r"(p->thread.soft[20]));
	__asm__ __volatile__("cprgr  %0, <0, 0x15>\n\r"
	                        :"=r"(p->thread.soft[21]));
	__asm__ __volatile__("cprgr  %0, <0, 0x16>\n\r"
	                        :"=r"(p->thread.soft[22]));
	__asm__ __volatile__("cprgr  %0, <0, 0x17>\n\r"
	                        :"=r"(p->thread.soft[23]));
	__asm__ __volatile__("cprgr  %0, <0, 0x18>\n\r"
	                        :"=r"(p->thread.soft[24]));
	__asm__ __volatile__("cprgr  %0, <0, 0x19>\n\r"
						:"=r"(p->thread.soft[25]));
	__asm__ __volatile__("cprgr  %0, <0, 0x1a>\n\r"
							:"=r"(p->thread.soft[26]));
	__asm__ __volatile__("cprgr  %0, <0, 0x1b>\n\r"
                                :"=r"(p->thread.soft[27]));	
	__asm__ __volatile__("cprgr  %0, <0, 0x20>\n\r"
                                :"=r"(p->thread.hard[0]));
	__asm__ __volatile__("cprgr  %0, <0, 0x21>\n\r"
                                :"=r"(p->thread.hard[1]));
	__asm__ __volatile__("cprgr  %0, <0, 0x22>\n\r"
                                :"=r"(p->thread.hard[2]));
	__asm__ __volatile__("cprgr  %0, <0, 0x23>\n\r"
                                :"=r"(p->thread.hard[3]));
	__asm__ __volatile__("cprgr  %0, <0, 0x24>\n\r"
                                :"=r"(p->thread.hard[4]));
	__asm__ __volatile__("cprgr  %0, <0, 0x25>\n\r"
                                :"=r"(p->thread.hard[5]));
	__asm__ __volatile__("cprgr  %0, <0, 0x26>\n\r"
                                :"=r"(p->thread.hard[6]));
	__asm__ __volatile__("cprgr  %0, <0, 0x27>\n\r"
                                :"=r"(p->thread.hard[7]));
	__asm__ __volatile__("cprgr  %0, <0, 0x28>\n\r"
                                :"=r"(p->thread.hard[8]));
	__asm__ __volatile__("cprgr  %0, <0, 0x29>\n\r"
                                :"=r"(p->thread.hard[9]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2a>\n\r"
                                :"=r"(p->thread.hard[10]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2b>\n\r"
                                :"=r"(p->thread.hard[11]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2c>\n\r"
                                :"=r"(p->thread.hard[12]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2d>\n\r"
                                :"=r"(p->thread.hard[13]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2e>\n\r"
                                :"=r"(p->thread.hard[14]));
	__asm__ __volatile__("cprgr  %0, <0, 0x2f>\n\r"
                                :"=r"(p->thread.hard[15]));
	__asm__ __volatile__("cprgr  %0, <0, 0x30>\n\r"
                                :"=r"(p->thread.hard[16]));
	__asm__ __volatile__("cprgr  %0, <0, 0x31>\n\r"
                                :"=r"(p->thread.hard[17]));
	__asm__ __volatile__("cprgr  %0, <0, 0x32>\n\r"
                                :"=r"(p->thread.hard[18]));
	__asm__ __volatile__("cprgr  %0, <0, 0x33>\n\r"
                                :"=r"(p->thread.hard[19]));
	__asm__ __volatile__("cprgr  %0, <0, 0x34>\n\r"
                                :"=r"(p->thread.hard[20]));
	__asm__ __volatile__("cprgr  %0, <0, 0x35>\n\r"
                                :"=r"(p->thread.hard[21]));
	__asm__ __volatile__("cprgr  %0, <0, 0x36>\n\r"
                                :"=r"(p->thread.hard[22]));
	__asm__ __volatile__("cprgr  %0, <0, 0x37>\n\r"
                                :"=r"(p->thread.hard[23]));
	__asm__ __volatile__("cprgr  %0, <0, 0x38>\n\r"
                                :"=r"(p->thread.hard[24]));
	__asm__ __volatile__("cprgr  %0, <0, 0x39>\n\r"
                                :"=r"(p->thread.hard[25]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3a>\n\r"
                                :"=r"(p->thread.hard[26]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3b>\n\r"
                                :"=r"(p->thread.hard[27]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3c>\n\r"
                                :"=r"(p->thread.hard[28]));
	__asm__ __volatile__("cprgr  %0, <0, 0x3d>\n\r"
                                :"=r"(p->thread.hard[29]));
	__asm__ __volatile__("cprgr  %0, <0, 0x40>\n\r"
                                :"=r"(p->thread.extend[0]));
	__asm__ __volatile__("cprgr  %0, <0, 0x41>\n\r"
                                :"=r"(p->thread.extend[1]));
	__asm__ __volatile__("cprgr  %0, <0, 0x42>\n\r"
                                :"=r"(p->thread.extend[2]));
	__asm__ __volatile__("cprgr  %0, <0, 0x43>\n\r"
                                :"=r"(p->thread.extend[3]));
	__asm__ __volatile__("cprgr  %0, <0, 0x44>\n\r"
                                :"=r"(p->thread.extend[4]));
	 __asm__ __volatile__("cprgr  %0, <0, 0x45>\n\r"
	                        :"=r"(p->thread.extend[5]));
	__asm__ __volatile__("cprgr  %0, <0, 0x46>\n\r"
	                        :"=r"(p->thread.extend[6]));
	__asm__ __volatile__("cprgr  %0, <0, 0x47>\n\r"
	                        :"=r"(p->thread.extend[7]));
	__asm__ __volatile__("cprgr  %0, <0, 0x48>\n\r"
	                        :"=r"(p->thread.extend[8]));
	__asm__ __volatile__("cprgr  %0, <0, 0x49>\n\r"
	                        :"=r"(p->thread.extend[9]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4a>\n\r"
	                        :"=r"(p->thread.extend[10]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4b>\n\r"
	                        :"=r"(p->thread.extend[11]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4c>\n\r"
	                        :"=r"(p->thread.extend[12]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4d>\n\r"
	                        :"=r"(p->thread.extend[13]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4e>\n\r"
	                        :"=r"(p->thread.extend[14]));
	__asm__ __volatile__("cprgr  %0, <0, 0x4f>\n\r"
	                        :"=r"(p->thread.extend[15]));
	__asm__ __volatile__("cprgr  %0, <0, 0x50>\n\r"
	                        :"=r"(p->thread.extend[16]));
	__asm__ __volatile__("cprgr  %0, <0, 0x51>\n\r"
	                        :"=r"(p->thread.extend[17]));
	__asm__ __volatile__("cprgr  %0, <0, 0x52>\n\r"
	                        :"=r"(p->thread.extend[18]));
	__asm__ __volatile__("cprgr  %0, <0, 0x53>\n\r"
	                        :"=r"(p->thread.extend[19]));
	__asm__ __volatile__("cprgr  %0, <0, 0x54>\n\r"
	                        :"=r"(p->thread.extend[20]));
	__asm__ __volatile__("cprgr  %0, <0, 0x55>\n\r"
	                        :"=r"(p->thread.extend[21]));
	__asm__ __volatile__("cprgr  %0, <0, 0x56>\n\r"
	                        :"=r"(p->thread.extend[22]));
	__asm__ __volatile__("cprgr  %0, <0, 0x57>\n\r"
	                        :"=r"(p->thread.extend[23]));
	__asm__ __volatile__("cprgr  %0, <0, 0x58>\n\r"
	                        :"=r"(p->thread.extend[24]));
	__asm__ __volatile__("cprgr  %0, <0, 0x59>\n\r"
                                :"=r"(p->thread.extend[25]));
#endif
#if defined(CONFIG_CPU_HAS_DSP) || defined(__CK810__)
	__asm__ __volatile__("mfhi    %0 \n\r"
		                 "mflo    %1 \n\r"
		                 :"=r"(p->thread.hi), "=r"(p->thread.lo)
		                 : );
#endif
	preempt_enable();

	*childregs = *regs;
	/*Return 0 for subprocess when return from fork(),vfork(),clone()*/
	childregs->a0 = 0;

	childstack = ((struct switch_stack *) childregs) - 1;
	memset(childstack, 0, sizeof(struct switch_stack));
	childstack->r15 = (unsigned long)ret_from_fork;

	p->thread.usp = usp;
	p->thread.ksp = (unsigned long)childstack;
	
	if (clone_flags & CLONE_SETTLS)
	{
		ti->tp_value = regs->regs[0];	
#ifdef CONFIG_CPU_CSKYV2
		childregs->exregs[15] = regs->regs[0];
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

/*
 * fill in the user structure for a core dump..
 *
 * this function is not used in linux 2.6
 */
void dump_thread(struct pt_regs * regs, struct user * dump)
{
}

asmlinkage int csky_execve(const char __user *name, 
	const char __user * __user *argv, 
	const char __user * __user *envp, struct pt_regs *regs)
{
	/* struct pt_regs *regs; */ /* commended by sunkang 2004.11.18 */
	int error;
	struct filename *filename;
        
	filename = getname(name);
	error = PTR_ERR(filename);
	if (IS_ERR(filename))
		return error;
	error = do_execve(filename, argv, envp);
	putname(filename);
	return error;
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

