#include <linux/linkage.h>
#include <hal/entry.h>
#include <asm/errno.h>
#include <asm/setup.h>
#include <asm/traps.h>
#include <hal/pgtable-bits.h>
#include <asm/unistd.h>
#include <asm/asm-offsets.h>
#include <linux/threads.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/thread_info.h>
#include <asm/fpu.h>

#define PTE_HALF        0
#define PTE_SIZE        4
#define PTE_BIT         2
#define PTEP_INDX_MSK	0xff8
#define PTE_INDX_MSK    0xffc
#define PTE_INDX_SHIFT  10
#define _PGDIR_SHIFT    22
#define THREADSIZE_MASK_BIT 13

/*
 * Make sure our user space atomic helper(trap 2) is restarted
 * if it was interrupted in a critical region. Here we
 * perform a quick test inline since it should be false
 * 99.9999% of the time. The rest is done out of line.
 *
 * This macro is used in tlbmodified.
 */
.macro kuser_cmpxchg_check
	mfcr    a0, epc
	btsti	a0, 31			/* is in super user mode?    if yes -=> call kuser_cmpxchg_fixup */
	bf	1f
	jbsr	kuser_cmpxchg_fixup
1:
.endm

.export system_call
.export buserr
.export trap
.export alignment
.export inthandler
.export autohandler
.export fasthandler

.export fastautohandler
.export sys_fork, sys_clone
.export sw_usp
.export sw_ksp

.export handle_tlbinvalidl
.export handle_tlbmodified
.export handle_tlbmissinst
.export handle_tlbmissdata
.export tlbinvalidl
.export tlbinvalids
.export tlbmiss
.export readtlbinvalid
.export writetlbinvalid
.export handle_fpe
.export handle_illegal

.import irq_stat

#ifndef CONFIG_MMU_HARD_REFILL
.import pgd_current
#endif

.data
sw_ksp:
.long 0
sw_usp:
.long 0

.text

/*
 * Tlbinvalidl exception handle routine.
 */
ENTRY(handle_tlbinvalidl)
tlbinvalidl:
	mtcr    a3, ss2
	mtcr    r6, ss3
	mtcr    a2, ss4

	SET_CP_MMU
#ifdef CONFIG_MMU_HARD_REFILL
	RD_PGDR	r6
	bclri   r6, 0
	lrw	a3, PHYS_OFFSET
	subu	r6, a3
	bseti	r6, 31
#else
	lrw     r6, (pgd_current)
	ldw     r6, (r6)
#endif
	RD_MEH	    a3
	mov     a2, a3
   	lsri    a2, _PGDIR_SHIFT
	lsli    a2, 2
	addu    r6, a2
	ldw     r6, (r6)
#ifdef CONFIG_MMU_HARD_REFILL
	lrw	a2, PHYS_OFFSET
	subu	r6, a2
	bseti	r6, 31
#endif

	lsri    a3, PTE_INDX_SHIFT
	lrw     a2, PTE_INDX_MSK
	and     a3, a2
	addu    r6, a3
	ldw     a3, (r6)
	bgeni   a2, 31            /* move 0x80000000 to a2 */
	WR_MCIR	a2
	movi    a2, (_PAGE_PRESENT | _PAGE_READ)
	and     a3, a2
	cmpne   a3, a2
	bt      readtlbinvalid   /* PTE not present, jump to fix it. */

	/* PTE present, now make it valid */
	ldw     a3, (r6)
#ifdef __CSKYABIV1__
	bgeni   a2, 7         /* a2 = (_PAGE_VALID | _PAGE_ACCESSED) */
 	bseti   a2, 3
#else
	movi    a2, (_PAGE_VALID | _PAGE_ACCESSED)
#endif
	or      a3, a2
	stw     a3, (r6)

	/*
	 * Below, fill a jTLB with two PTEs of which we have set one above.
	 * When do this, we make sure set Entrylo0 with the low PTE in Page
	 * Table, and Entrylo1 with the high one.
	 */
	bclri   r6, PTE_BIT
#ifdef __CSKYABIV1__
	ldw     a2, (r6, 4)
	lsri    a2, 6
	WR_MEL1 a2
	ldw     a2, (r6)
	lsri    a2, 6
	WR_MEL0 a2
#else
	ldw     a2, (r6, 4)
	WR_MEL1 a2
	ldw     a2, (r6)
	WR_MEL0    a2
#endif

	RD_MIR  a3         /* Read MIR */
	bgeni   a2, 29     /* Use write index by default */
	btsti   a3, 31     /* Is probe success ? */
	bf      1f
	bgeni   a2, 25
	WR_MCIR a2
	bgeni   a2, 28     /* If probe failed, invalid index and write random */

1:
	WR_MCIR a2
	mfcr    a3, ss2
	mfcr    r6, ss3
	mfcr    a2, ss4
	rte

readtlbinvalid:
	mfcr    a3, ss2
	mfcr    r6, ss3
	mfcr    a2, ss4
	SAVE_ALL

	SET_CP_MMU
	RD_MEH	a3
	bmaski  r8, 12
	andn    a3, r8             /* r8 = !(0xfffff000) */
	mov     a2, a3
	psrset  ee, ie          /* Enable exception & interrupt */
	mov     a0, sp
	movi    a1, 0
	jbsr    do_page_fault
	movi    r11_sig, 0             /* r11 = 0, Not a syscall. */
	jmpi    ret_from_exception

/*
 * Tlbinvalids exception handle routine.
 */
ENTRY(handle_tlbinvalids)
tlbinvalids:
	mtcr    a3, ss2
	mtcr    r6, ss3
	mtcr    a2, ss4

	SET_CP_MMU
#ifdef CONFIG_MMU_HARD_REFILL
	RD_PGDR	r6
	bclri   r6, 0
	lrw	a3, PHYS_OFFSET
	subu	r6, a3
	bseti	r6, 31
#else
 	lrw 	r6, pgd_current
	ldw     r6, (r6)
#endif

	RD_MEH	a3
	mov     a2, a3
	lsri    a2, _PGDIR_SHIFT
	lsli    a2, 2
	addu    r6, a2
	ldw     r6, (r6)
#ifdef CONFIG_MMU_HARD_REFILL
	lrw	a2, PHYS_OFFSET
	subu	r6, a2
	bseti   r6, 31
#endif

	lsri    a3, PTE_INDX_SHIFT
	lrw     a2, PTE_INDX_MSK
	and     a3, a2
	addu    r6, a3
	ldw     a3, (r6)
	bgeni   a2, 31           /* TLB probe command, a2 = 0x80000000 */
	WR_MCIR	a2
	movi    a2, (_PAGE_PRESENT | _PAGE_WRITE)
	and     a3, a2
	xor     a3, a2
	cmpnei  a3, 0
	bt      writetlbinvalid  /* PTE not present, jump to fix it. */

	/* PTE resent, set it to be valid. */
	ldw     a3, (r6)

#ifdef __CSKYABIV1__
	/* a2 = (_PAGE_ACCESSED | _PAGE_MODIFIED | _PAGE_VALID | _PAGE_DIRTY) */
	movi    a2, 0x18
	bseti   a2, 7
	bseti   a2, 8
#else
	movi    a2, (_PAGE_ACCESSED | _PAGE_MODIFIED | _PAGE_VALID | _PAGE_DIRTY)
#endif

	or      a3, a2
	stw     a3, (r6)
	/*
	 * Below, fill a jTLB with two PTEs of which we have set one above.
	 * When do this, we make sure set Entrylo0 with the low PTE in Page
	 * Table, and Entrylo1 with the high one.
	 */
	bclri   r6, PTE_BIT
#ifdef __CSKYABIV1__
	ldw     a2, (r6,4)
	lsri    a2, 6
	WR_MEL1	a2
	ldw 	a2, (r6)
	lsri	a2, 6
	WR_MEL0	a2
#else
	ldw     a2, (r6,4)
	WR_MEL1 a2
	ldw     a2, (r6)
	WR_MEL0 a2
#endif

	RD_MIR  a3         /* Read MIR */
	bgeni   a2, 29     /* Use write index by default */
	btsti   a3, 31     /* Is probe success ? */
	bf      1f
	bgeni   a2, 25
	WR_MCIR a2
	bgeni   a2, 28     /* If probe failed, invalid index and write random */

1:
	WR_MCIR	a2

	mfcr    a3, ss2
	mfcr    r6, ss3
	mfcr    a2, ss4
	rte

writetlbinvalid:
	mfcr    a3, ss2
	mfcr    r6, ss3
	mfcr    a2, ss4
	SAVE_ALL

	SET_CP_MMU
	RD_MEH	    a3
	bmaski  r8, 12
	andn    a3, r8          /* r8 = !(0xfffff000) */
	mov     a2, a3
	psrset  ee, ie          /* Enable exception & interrupt */
	mov     a0, sp
	movi    a1, 1
	jbsr    (do_page_fault)
	movi    r11_sig, 0             /* r11 = 0, Not a syscall. */
	jmpi    (ret_from_exception)

/*
 * Tlbmiss exception handle routine.
 */
ENTRY(handle_tlbmiss)
tlbmiss:
#ifndef CONFIG_MMU_HARD_REFILL
	lrw     a0, (pgd_current)
	ldw     a0, (a0)

	SET_CP_MMU
	RD_MEH  a1
#ifdef __CSKYABIV1__
	mov     a2, a1
	lsri    a2, _PGDIR_SHIFT
	ixw     a0, a2
	ldw     a0, (a0)

	lsri    a1, PTE_INDX_SHIFT
	lrw     a2, PTE_INDX_MSK
	and     a1, a2
	addu    a0, a1
	bclri   a0, PTE_BIT
	ldw     a1, (a0)
	lsri    a1, 6
	WR_MEL0 a1
	ldw     a1, (a0, 4)
	lsri    a1, 6
	WR_MEL1 a1
#else
#error ck8xx only hardrefill for tlbmiss, because some no shadow regs
#endif

	bgeni   a1, 28           /* TLB write random command, r5 = 0x10000000 */
	WR_MCIR a1
#endif
	/*
	 * clear TP in psr[13]
	 */
	mfcr    a1, epsr
	bclri   a1, 13
	mtcr    a1, epsr
	rte

/*
 * Tlbmodified exception handle routine.
 */
ENTRY(handle_tlbmodified)
	mtcr    a3, ss2
	mtcr    r6, ss3
	mtcr    a2, ss4

	/*
	 * clear TP in psr[13]
	 */
	mfcr    a3, epsr
	bclri   a3, 13
	mtcr    a3, epsr

	SET_CP_MMU
#ifdef CONFIG_MMU_HARD_REFILL
	RD_PGDR	r6
	bclri   r6, 0
	lrw	a3, PHYS_OFFSET
	subu	r6, a3
	bseti   r6, 31
#else
	lrw     r6, (pgd_current)
	ldw     r6, (r6)
#endif

	RD_MEH  a3
	mov     a2, a3
	lsri    a2, _PGDIR_SHIFT
	lsli    a2, 2
	addu    r6, a2

	/*
	 * get pte table to r6
	 */
	ldw     r6, (r6)
#ifdef CONFIG_MMU_HARD_REFILL
	lrw	a2, PHYS_OFFSET
	subu	r6, a2
	bseti   r6, 31
#endif

	lsri    a3, PTE_INDX_SHIFT
	lrw     a2, PTE_INDX_MSK
	and     a3, a2
	addu    r6, a3
	ldw     a3, (r6)/* get pte to a3*/

	bgeni   a2, 31	/* TLB probe command, a2 = 0x80000000 */
	WR_MCIR	a2	/* find faulting entry */

	/*
	 * if _PAGE_WRITE == 0, goto tlbmodified.
	 */
	movi    a2, _PAGE_WRITE
	and     a3, a2
	cmpnei  a3, 0
	bf      tlbmodified
	ldw     a3, (r6)

	/*
	 * Present and writable bits set, set accessed and dirty bits.
	 * a2 = (_PAGE_ACCESSED | _PAGE_MODIFIED | _PAGE_VALID | _PAGE_DIRTY)
	 */
#ifdef __CSKYABIV1__
	movi    a2, 0x18
	bseti   a2, 7
	bseti   a2, 8
#else
	movi    a2, (_PAGE_ACCESSED | _PAGE_MODIFIED | _PAGE_VALID | _PAGE_DIRTY)
#endif
	or      a3, a2
	stw     a3, (r6)

	/* Now reload the entry into the tlb. */
	bclri   r6, PTE_BIT
#ifdef __CSKYABIV1__
	ldw     a2, (r6, 4)
	lsri    a2, 6
	WR_MEL1	a2
	ldw     a2, (r6)
	lsri    a2, 6
	WR_MEL0	a2
#else
	ldw     a2, (r6, 4)
	WR_MEL1 a2
	ldw     a2, (r6)
	WR_MEL0 a2
#endif

	RD_MIR  a3         /* Read MIR */
	bgeni   a2, 29     /* Use write index by default */
	btsti   a3, 31     /* Is probe success ? */
	bf      1f
	bgeni   a2, 25
	WR_MCIR a2
	bgeni   a2, 28     /* If probe failed, invalid index and write random */

1:
	WR_MCIR	a2

	mfcr    a3, ss2
	mfcr    r6, ss3
	mfcr    a2, ss4
	rte

tlbmodified:
	mfcr    a3, ss2
	mfcr    r6, ss3
	mfcr    a2, ss4
	SAVE_ALL
	kuser_cmpxchg_check

	SET_CP_MMU
	RD_MEH	a3
	bmaski  r8, 12
	andn    a3, r8          /* a3 = a3 & (~0xfff) */
	mov     a2, a3
	psrset  ee, ie          /* Enable exception & interrupt */
	mov     a0, sp
	movi    a1, 1
	jbsr    (do_page_fault)
	movi    r11_sig, 0             /* r11 = 0, Not a syscall. */
	jmpi    (ret_from_exception)

/*
 * This function is used to handle access exception.
 */
ENTRY(buserr)
	SAVE_ALL
	SET_SMOD_MMU_CP15
	movi    r11_sig, 0        /* r11 = 0, Not a syscall. use in signal handle */
	mov     a0, sp            /* Stack address is arg[0] */
	jbsr    buserr_c          /* Call C level handler */
	jmpi    ret_from_exception

ENTRY(system_call)
	SAVE_ALL
	SET_SMOD_MMU_CP15

	/*
	 * Do not use r2-r7 here, because the arguments are saved in r2-r6
	 * and the syscall number is saved in syscallid when the exception is a
	 * systemcall.
	 * Use temp regs instead
	 *
	 * When excuting a trap instruction, the pc does not increase.
	 * The pc should
	 * be increased manully and save in epc register.
	 */
	mfcr    r13, epc                /* Get the trap point */

#if defined(__CSKYABIV1__)
	addi    r13, 2                  /* Increase the epc */
#elif defined(__CSKYABIV2__)
	addi    r13, 4                  /* Increase the epc, because the Instruct "trap x" in CK ISA V2 is 32 bit */
#endif

	mtcr    r13, epc                /* Save return point */
	stw     r13, (sp)               /* Save it in stack*/
	psrset  ee, ie                 /* Enable Exception & interrupt */

	/* Stack frame for syscall, origin call set_esp0 */
	mov     r12, sp

	bmaski  r11, 13
	andn    r12, r11
	bgeni   r11, 9
	addi    r11, 32
	addu    r12, r11
	st      sp, (r12, 0)

	lrw     r11, __NR_syscalls
	cmphs   syscallid, r11                 /* Check nr of syscall */
	bt      ret_from_exception

	lrw     r13, sys_call_table
	ixw     r13, syscallid                 /* Index into syscall table */
	ldw     r11, (r13)               /* Get syscall function */
	cmpnei  r11, 0                  /* Check for not null */
	bf      ret_from_exception

	mov     r9, sp				 /* Get task pointer */
	bmaski  r10, THREADSIZE_MASK_BIT
	andn    r9, r10                      /* Get thread_info */
	ldw     r8, (r9, TINFO_FLAGS)       /* Get thread_info.flags value */
	btsti   r8, TIF_SYSCALL_TRACE       /* Check if TIF_SYSCALL_TRACE set */
	bt      1f
#if defined(__CSKYABIV2__)
	subi    sp, 8
	stw  	r5, (sp, 0x4)
	stw  	r4, (sp, 0x0)
	jsr     r11                      /* Do system call */
	addi 	sp, 8
#else
	jsr     r11
#endif
	stw     a0, (sp, LSAVE_A0)      /* Save return value */
	jmpi    ret_from_exception

1:
	movi    a0, 0                   /* enter system call */
	mov     a1, sp                  /* right now, sp --> pt_regs */
	jbsr    syscall_trace
	/* Prepare args before do system call */
	ldw     a0, (sp, LSAVE_A0)
	ldw     a1, (sp, LSAVE_A1)
	ldw     a2, (sp, LSAVE_A2)
	ldw     a3, (sp, LSAVE_A3)
#if defined(__CSKYABIV2__)
	subi    sp, 8
	stw     r5, (sp, 0x4)
	stw     r4, (sp, 0x0)
#else
	ldw     r6, (sp, LSAVE_REGS0)
	ldw     r7, (sp, LSAVE_REGS1)
#endif
	jsr     r11                     /* Do system call */
#if defined(__CSKYABIV2__)
	addi    sp, 8
#endif
	stw     a0, (sp, LSAVE_A0)     /* Save return value */

	movi    a0, 1                   /* leave system call */
	mov     a1, sp                  /* right now, sp --> pt_regs */
	jbsr    syscall_trace

syscall_exit_work:
	ld       syscallid, (sp, 8)     /* get psr, is user mode? */
	btsti    syscallid, 31
	bt       2f

	jmpi     resume_userspace

2:      RESTORE_ALL

ENTRY(ret_from_kernel_thread)
	jbsr     schedule_tail
	mov	 a0, r8
	jsr	 r9
	jbsr     ret_from_exception


ENTRY(ret_from_fork)
	jbsr     schedule_tail
	mov      r9, sp				 /* Get task pointer */
	bmaski   r10, THREADSIZE_MASK_BIT
	andn     r9, r10                     /* Get thread_info */
	ldw      r8, (r9, TINFO_FLAGS)       /* Get thread_info.flags value */
	movi     r11_sig, 1                  /* is a syscall */
	btsti    r8, TIF_SYSCALL_TRACE       /* Check if TIF_SYSCALL_TRACE set */
	bf       3f
	movi     a0, 1                       /* leave system call */
	mov      a1, sp                      /* right now, sp --> pt_regs */
	jbsr     syscall_trace
3:
	jbsr     ret_from_exception

ret_from_exception:
	ld       syscallid, (sp,8)     /* get psr, is user mode? */
	btsti    syscallid, 31
	bt       1f
	/*
	 * Load address of current->thread_info, Then get address of task_struct
	 * Get task_needreshed in task_struct
	 */
	mov     r9, sp     					 /* Get current stack  pointer */
	bmaski  r10, THREADSIZE_MASK_BIT
	andn    r9, r10                      /* Get task_struct */

resume_userspace:
	ldw      r8, (r9, TINFO_FLAGS)
	andi     r8, (_TIF_SIGPENDING | _TIF_NOTIFY_RESUME | _TIF_NEED_RESCHED)
	cmpnei   r8, 0
	bt       exit_work
1:  RESTORE_ALL

exit_work:
	mov      a0, sp                 /* Stack address is arg[0] */
	jbsr     set_esp0               /* Call C level */
	btsti    r8, TIF_NEED_RESCHED
	bt       work_resched
	cmpnei   r8, 0		/* If thread_info->flag is empty, RESTORE_ALL. */
	bf       1b
	mov      a1, sp
	mov      a0, r8
	mov      a2, r11_sig        /* syscall? */
	btsti    r8, TIF_SIGPENDING /* delivering a signal? */
	clrt     r11_sig            /* prevent further restarts(set r11 = 0) */
	jbsr     do_notify_resume	/* do signals */
	br       resume_userspace

work_resched:
	lrw      syscallid, ret_from_exception
	mov      r15, syscallid                /* Return address in link */
	jmpi     schedule

ENTRY(sys_rt_sigreturn)
	movi	r11_sig, 0
	jmpi	do_rt_sigreturn

/*
 * Common trap handler. Standard traps come through here first
 */

ENTRY(trap)
	SAVE_ALL
	SET_SMOD_MMU_CP15

	movi     r11_sig, 0             /* r11 = 0, Not a syscall. */
	mfcr     a0, psr                /* Get psr register */
	lsri     a0, 16                 /* Get vector in base 8 bits */
	sextb    a0                     /* Fill upper bytes with zero */
	mov      a1, sp                 /* Push Stack pointer arg */
	jbsr     trap_c                 /* Call C-level trap handler */
	jmpi     ret_from_exception

/*
 * Common illegal handler.
 */

ENTRY(handle_illegal)
	SAVE_ALL
	psrset   ee
	movi     r11_sig, 0             /* r11 = 0, Not a syscall. */
	mov      a0, sp                 /* Push Stack pointer arg */
	jbsr     handle_illegal_c       /* Call C-level trap handler */
	jmpi     ret_from_exception

/*
 * Alignment_exception handler.
 */
ENTRY(alignment)
	SAVE_ALL
	SET_SMOD_MMU_CP15
	psrset   ee                     /* Enable Exception */
	movi     r11_sig, 0             /* r11 = 0, Not a syscall. */
	mov      a0, sp                 /* Push Stack pointer arg */
	jbsr     alignment_c            /* Call C-level align exception handler */
	jmpi     ret_from_exception

ENTRY(trap1)
#if defined(__CSKYABIV1__)
	mtcr     sp, ss1
	mfcr     sp, ss0
	mtcr     a1, ss4
#elif defined(__CSKYABIV2__)
	subi     sp, 8
	stw      a1,(sp)
#endif
	mfcr     a1, epc                /* Get the trap point */
#if defined(__CSKYABIV1__)
	addi     a1, 2                  /* Increase the epc */
#elif defined(__CSKYABIV2__)
	addi     a1, 4                  /* Increase the epc, because the Instruct "trap x" in CK ISA V2 is 32 bit */
#endif
	mtcr     a1, epc                /* Save return point */

	movi     a1, 0x32
	mtcr     a1, cr17
#if defined(__CSKYABIV1__)
	mfcr     a1, ss4
	mtcr     sp, ss0
	mfcr     sp, ss1
#elif defined(__CSKYABIV2__)
	ldw      a1,(sp)
	addi     sp, 8
#endif
	rte

/*
 * exception  trap 2 use to cmpxchg, reference prototype:
 *      int __kernel_cmpxchg(int oldval, int newval, int *ptr)
 *
 * If *ptr != oldval, direct return 1,
 * else set *ptr = newval, then return 0.
 *
 * Input:
 *      a0 = oldval
 *      a1 = newval
 *      a2 = ptr
 * Output:
 *      a0 = returned value (zero or non-zero)
 *
 * Clobbered:
 *      a3!
 *
 * Attention: trap 2 is not a atomic function!
 * The "stw a1, (a2)" may produce tlbmodified exception, then may cause schedule.
 * So return back to "ldw" after tlbmodified, if stw was interrupted.
 */

ENTRY(trap2)
#if defined(__CSKYABIV1__)
	mtcr     sp, ss1
	mfcr     sp, ss0
#endif
	mfcr     a3, epc		/* Get the trap point */
#if defined(__CSKYABIV1__)
	addi     a3, 2			/* Increase the epc */
#elif defined(__CSKYABIV2__)
	addi     a3, 4			/* Increase the epc, because the Instruct "trap x" in CK ISA V2 is 32 bit */
#endif
	subi     sp, 8
	stw      a3, (sp, 0)		/* need to save epc to sp */
	mfcr     a3, epsr
	stw      a3, (sp, 4)		/* need to save epsr to sp */

	psrset   ee			/* Enable Exception for tlb exception */

1:					/* "1" is for kuser_cmpxchg_fixup */
	ldw      a3, (a2)
	cmpne    a0, a3
	bt       3f

2:					/* "2" is for kuser_cmpxchg_fixup */
	stw      a1, (a2)
3:
	mvc      a0			/* return value */
	ldw      a3, (sp, 0)		/* restore epc */
	mtcr     a3, epc
	ldw      a3, (sp, 4)		/* restore epsr */
	mtcr     a3, epsr
	addi     sp, 8
#if defined(__CSKYABIV1__)
	mtcr     sp, ss0
	mfcr     sp, ss1
#endif
	rte

/*
 *  Called from kuser_cmpxchg_check macro.
 *  Input:
 *  	a0 = address of interrupted insn(epc).
 *  	1b = first critical insn, 2b = last critical insn.
 *  Output:
 *	None.
 *
 *  Clobbered:
 *      a0, a1!
 *
 *  If a2 == 2b then saved pt_regs's epc is set to 1b.
 */
ENTRY(kuser_cmpxchg_fixup)
	lrw	a1, 2b
	cmpne	a1, a0
	bt	1f
// FIXME: abiv2 should use "subi    a1, (2b-1b)", but assembler does not support now!
#ifdef __CSKYABIV2__
	lrw	a1, 1b
#else
	subi	a1, (2b-1b)		/* get 1b */
#endif
	stw	a1, (sp, 0)		/* set pt_reg's epc = 1b */
1:
	rts

/*
 * Reference prototype:
 *  int __kernel_get_tls(int addr)
 * Input:
 *  none 
 * Output:
 *  r2 = TLS value
 * Clobbered:
 *  none
 * Definition and user space usage example:
 *  typedef int (__kernel_get_tls_t)(int addr);
 * Get the TLS value as previously set via the set_thread_area syscall.
 * This could be used as follows:
 * #define __kernel_get_tls() \
 *  ({ register unsigned int __result asm("a0"); \
 *         asm( "trap  3" \
 *          : "=r" (__result) : :  ); \
 *     __result; })
 */
ENTRY(trap3)                        /*added for get tls*/
#if defined(__CSKYABIV1__)
	mtcr     sp, ss1
	mfcr     sp, ss0
#endif

	subi     sp, 8                  /* because sp may align wich 0x2000 */
	mfcr     a0, epc                /* Get the trap point */
#if defined(__CSKYABIV1__)
	addi     a0, 2                  /* Increase the epc */
#elif defined(__CSKYABIV2__)
	addi     a0, 4                  /* Increase the epc, because the Instruct "trap x" in CK ISA V2 is 32 bit */
#endif
	mtcr     a0, epc                /* Save return point */

	bmaski   a0, (PAGE_SHIFT + 1)   /* kernel stack is 2*page if page is 4k */
	not      a0
	and      a0, sp                 /* thread_info local in bottom of stack */

	ldw      a0, (a0, TINFO_TP_VALUE) /* get tls */

	addi     sp, 8
#if defined(__CSKYABIV1__)
	mtcr     sp, ss0
	mfcr     sp, ss1
#endif
	rte

/*
 * handle FPU exception.
 */
ENTRY(handle_fpe)
	SAVE_ALL
	/* Clear FPU exception state */
#if defined(CONFIG_CPU_HAS_FPU)
	mfcr      a0, cr<2, 2>	       /* fpu fesr is cr<2,2> in CSKY_CPUV2 */
	movi      r11_sig, 0           /* r11 = 0, Not a syscall. */
	mov       a1, sp               /* Push Stack pointer arg */
	jbsr      handle_fpe_c         /* Call C-level fpe handler */
#endif
	jmpi      ret_from_exception

/*
 * handle interrupt.
 */
ENTRY(inthandler)
	SAVE_ALL
	SET_SMOD_MMU_CP15
	psrset	ee				/* Enable exceptions */

	movi	r11_sig, 0			/* r11 = 0, Not a syscall. */

#ifdef CONFIG_PREEMPT
	mov	r9, sp				/* Get current stack  pointer */
	bmaski  r10, THREADSIZE_MASK_BIT
	andn    r9, r10				/* Get thread_info */

	ldw      r8, (r9, TINFO_PREEMPT)
	addi     r8, 1
	stw      r8, (r9, TINFO_PREEMPT)
#endif
	mfcr     a0, psr                /* Get PSR register */
	lsri     a0, 16                 /* Get vector in 7 bits */
	sextb    a0                     /* Fill upper bytes with zero */
	subi     a0, 32                 /* Real irq nomber need sub VEC offset(32)*/
	mov      a1, sp                 /* arg[1] is stack pointer */
	jbsr     csky_do_IRQ          /* Call handler */

#ifdef CONFIG_PREEMPT
	subi     r8, 1
	stw      r8, (r9, TINFO_PREEMPT)
	cmpnei   r8, 0
	bt       2f
	ldw      r8, (r9, TINFO_FLAGS)
	btsti    r8, TIF_NEED_RESCHED
	bf       2f
1:
	jbsr     preempt_schedule_irq   /* irq en/disable is done inside */
	ldw      r7, (r9, TINFO_FLAGS)  /* get new tasks TI_FLAGS */
	btsti    r7, TIF_NEED_RESCHED
	bt       1b                     /* go again */
#endif
2:
	jmpi     ret_from_exception

/*
 * This is the auto-vectored interrupt handler (for all hardware interrupt
 * sources). It figures out the vector number and calls the appropriate
 * interrupt service routine directly. This is for auto-vectored normal
 * interrupts only.
 *
 */

ENTRY(autohandler)
	SAVE_ALL
	SET_SMOD_MMU_CP15
	psrset  ee       // enable exception
	movi    r11_sig, 0                   /* r11 = 0, Not a syscall. */

#ifdef CONFIG_PREEMPT
	mov     r9, sp                       /* Get current stack  pointer */
	bmaski  r10, THREADSIZE_MASK_BIT
	andn    r9, r10                      /* Get thread_info */

	/*
	 * Get task_struct->stack.preempt_count for current,
	 * and increase 1.
	 */
	ldw      r8, (r9, TINFO_PREEMPT)
	addi     r8, 1
	stw      r8, (r9, TINFO_PREEMPT)
#endif

	mov      a0, sp                      /* arg[0] is stack pointer */
	jbsr     csky_do_auto_IRQ          /* Call handler */

#ifdef CONFIG_PREEMPT
	subi     r8, 1
	stw      r8, (r9, TINFO_PREEMPT)
	cmpnei   r8, 0
	bt       2f
	ldw      r8, (r9, TINFO_FLAGS)
	btsti    r8, TIF_NEED_RESCHED
	bf       2f
1:
	jbsr     preempt_schedule_irq   /* irq en/disable is done inside */
	ldw      r7, (r9, TINFO_FLAGS)  /* get new tasks TI_FLAGS */
	btsti    r7, TIF_NEED_RESCHED
	bt       1b                     /* go again */
#endif
2:
	jmpi     ret_from_exception

/*
 * a0 =  prev task_struct *
 * a1 =  next task_struct *
 * a0 =  return next
 */
ENTRY(__switch_to)
	lrw      a3, TASK_THREAD        /* struct_thread offset in task_struct */
	addu     a3, a0                 /* a3 point to thread in prev task_struct */

	mfcr     a2, psr                /* Save PSR value */
	stw      a2, (a3, THREAD_SR)    /* Save PSR in task struct */
	bclri    a2, 6                  /* Disable interrupts */
	mtcr     a2, psr

	SAVE_SWITCH_STACK

#if defined(__CSKYABIV2__)
	mfcr     r6, cr<14, 1>           /* Get current usp */
#else
	mfcr     r6, ss1                /* Get current usp */
#endif
	stw      r6, (a3, THREAD_USP)   /* Save usp in task struct */
	stw      sp, (a3, THREAD_KSP)   /* Save ksp in task struct */

#ifdef CONFIG_CPU_HAS_FPU
	FPU_SAVE_REGS
#endif

#if  defined(CONFIG_CPU_HAS_DSP) || defined(__CK810__)
	/* Save DSP regs */
	lrw      r10, THREAD_DSPHI
	add      r10, a3
	mfhi     r6
	mflo     r7
	stw      r6, (r10, 0)           /* THREAD_DSPHI */
	stw      r7, (r10, 4)           /* THREAD_DSPLO */
	mfcr     r6, cr14
	stw      r6, (r10, 8)           /* THREAD_DSPCSR */
#endif

	/* Set up next process to run */
	lrw      a3, TASK_THREAD	/* struct_thread offset in task_struct */
	addu     a3, a1			/* a3 point to thread in next task_struct */

	ldw      sp, (a3, THREAD_KSP)	/* Set next ksp */
	ldw      r6, (a3, THREAD_USP)	/* Set next usp */

#if defined(__CSKYABIV2__)
	mtcr     r6, cr<14, 1>           /* Get current usp */
#else
	mtcr     r6, ss1                /* Get current usp */
#endif

#ifdef CONFIG_CPU_HAS_FPU
	FPU_RESTORE_REGS
#endif

#if  defined(CONFIG_CPU_HAS_DSP) || defined(__CSKYABIV2__)
	lrw      r10, THREAD_DSPHI
	add      r10, a3
	ldw      r6, (r10, 8)   /* THREAD_DSPCSR */
#if defined(__CSKYABIV2__)
	mtcr     r6, cr14
#else
	/*
	 * Because bit 0 in CK610's cr14 is read only, we need to restore it by
	 * using special method
	 */
	btsti    r6, 0
	movi     r7, 0xf
	bf       1f
	bmaski   r7, 0           /* old is "lrw r7, 0xffffffff" */
1:
	mthi     r7
	mulua    r7, r7
#endif
	/* Restore DSP regs */
	ldw      r6, (r10, 0)    /* THREAD_DSPHI */
	ldw      r7, (r10, 4)    /* THREAD_DSPLO */
	mthi     r6
	mtlo     r7
#endif

	ldw      a2, (a3, THREAD_SR)    /* Set next PSR */
	mtcr     a2, psr

#if  defined(__CSKYABIV2__)
	/* set TLS register (r31) */
	addi     r7, a1, TASK_THREAD_INFO
	ldw      r31, (r7, TINFO_TP_VALUE)
#endif

	RESTORE_SWITCH_STACK

	rts
ENDPROC(__switch_to)
