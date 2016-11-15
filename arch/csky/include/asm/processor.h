#ifndef __ASM_CSKY_PROCESSOR_H
#define __ASM_CSKY_PROCESSOR_H

/*
 * Default implementation of macro that returns current
 * instruction pointer ("program counter").
 */
#define current_text_addr() ({ __label__ _l; _l: &&_l;})

#include <linux/bitops.h>
#include <asm/segment.h>
#include <asm/fpu.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/cache.h>

/*
 * CSKY does have an arch_pick_mmap_layout()
 */
#define HAVE_ARCH_PICK_MMAP_LAYOUT 1

struct cpuinfo_csky {
	unsigned long udelay_val;
	unsigned long asid_cache;
	/*
	 * Capability and feature descriptor structure for CSKY CPU
	 */
	unsigned long options;
	unsigned int processor_id[4];
	unsigned int fpu_id;
} __attribute__((aligned(SMP_CACHE_BYTES)));

extern struct cpuinfo_csky cpu_data[];
#define current_cpu_data cpu_data[smp_processor_id()]


/* read user stack pointer */
extern inline unsigned long rdusp(void) {
	register unsigned long usp;
#if defined(__CSKYABIV2__)
        __asm__ __volatile__("mfcr %0, cr<14, 1> \n\r" : "=r" (usp));
#else
        __asm__ __volatile__("mfcr %0, ss1\n\r" : "=r" (usp));
#endif
	return usp;
}

/* write user stack pointer
   Fix me: should not only update user stack pointer in ss1,
   the user stack pointer saved in stack frame should be update
   either.*/
extern inline void wrusp(unsigned long usp) {
#if defined(__CSKYABIV2__)
        __asm__ __volatile__("mtcr %0, cr<14, 1> \n\r" : : "r" (usp));
#else
        __asm__ __volatile__("mtcr %0, ss1\n\r" : : "r" (usp));
#endif
}

/*
 * User space process size: 2GB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.  TASK_SIZE
 * for a 64 bit kernel expandable to 8192EB, of which the current CSKY
 * implementations will "only" be able to use 1TB ...
 */
#define TASK_SIZE       0x7fff8000UL

#ifdef __KERNEL__
#define STACK_TOP       TASK_SIZE
#define STACK_TOP_MAX   STACK_TOP
#endif

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE      (TASK_SIZE / 3)

struct thread_struct {
	unsigned long  ksp;       /* kernel stack pointer */
	unsigned long  usp;       /* user stack pointer */
	unsigned long  sr;        /* saved status register */
	unsigned long  esp0;      /* points to SR of stack frame */
	/* FPU regs */
	unsigned long  fcr;       /* fpu control reg */
	unsigned long  fsr;       /* fpu status reg, nothing in CPU_CSKYV2 */
	unsigned long  fesr;      /* fpu exception status reg */
	unsigned long  fp[32];    /* fpu general regs.
 				      In CPU_CSKYV1(FPU): 32 regs of 32 bits
 				        fp[0] store fr0,
				        fp[1] store fr1...
 				      In CPU_CSKYV2(VFP): 16 regs of 64 bits
				        fp[0] store vr0 low 32 bits,
				        fp[1] store vr0 high 32 bits... */

	/* Other stuff associated with the thread. */
	unsigned long address;      /* Last user fault */
	unsigned long baduaddr;     /* Last kernel fault accessing USEG  */
	unsigned long error_code;
	unsigned long trap_no;
};

#define INIT_THREAD  { \
	.ksp = (unsigned long) init_thread_union.stack + THREAD_SIZE, \
	.sr = 0x8f000000, \
}

/*
 * Do necessary setup to start up a newly executed thread.
 *
 * pass the data segment into user programs if it exists,
 * it can't hurt anything as far as I can tell
 */
#define PS_USE_MODE  0x7fffffff

#define start_thread(_regs, _pc, _usp)					\
do {									\
	set_fs(USER_DS); /* reads from user space */			\
	(_regs)->pc = (_pc);						\
	(_regs)->regs[1] = 0; /* ABIV1 is R7, uClibc_main rtdl arg */	\
	(_regs)->regs[2] = 0;						\
	(_regs)->regs[3] = 0; /* ABIV2 is R7, use it? */		\
	(_regs)->sr &= PS_USE_MODE;					\
	wrusp(_usp);							\
} while(0)

/* Forward declaration, a strange C thing */
struct task_struct;

/* Free all resources held by a thread. */
static inline void release_thread(struct task_struct *dead_task)
{
}

/* Prepare to copy thread state - unlazy all lazy status */
#define prepare_to_copy(tsk)    do { } while (0)

extern int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);

#define copy_segments(tsk, mm)		do { } while (0)
#define release_segments(mm)		do { } while (0)
#define forget_segments()		do { } while (0)

extern unsigned long thread_saved_pc(struct task_struct *tsk);

unsigned long get_wchan(struct task_struct *p);

#define	KSTK_EIP(tsk)							\
({									\
	unsigned long eip = 0;						\
	if ((tsk)->thread.esp0 > PAGE_SIZE &&				\
	     MAP_NR((tsk)->thread.esp0) < max_mapnr)			\
		eip = ((struct pt_regs *) (tsk)->thread.esp0)->pc;	\
		eip;							\
})

#define	KSTK_ESP(tsk) ((tsk) == current ? rdusp() : (tsk)->thread.usp)

#define task_pt_regs(p) \
	((struct pt_regs *)(THREAD_SIZE + task_stack_page(p)) - 1)

#define cpu_relax() barrier()

#endif /* __ASM_CSKY_PROCESSOR_H */
