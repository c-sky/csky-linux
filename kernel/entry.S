/*
 *
 * linux/arch/csky/entry.S
 *
 * entry.S  contains the system-call and fault low-level handling routines.
 * This also contains the timer-interrupt handler, as well as all interrupts
 * and faults that can result in a task-switch.
 *
 * NOTE: This code handles signal-recognition, which happens every time
 * after a timer-interrupt and after each system call.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file README.legal in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Hangzhou C-SKY Microsystems co.,ltd. 
 *
 */

#include <linux/linkage.h>
#include <asm/entry.h>
#include <asm/errno.h>
#include <asm/setup.h>
#include <asm/traps.h>
#include <asm/pgtable-bits.h>
#include <asm/unistd.h>
#include <asm/asm-offsets.h>
#include <linux/threads.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/csky.h>
#include <asm/thread_info.h>
#include <asm/regdef.h>
#include <asm/prfl.h>
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
.export resume, ret_from_exception
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
.export sys_call_table

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
#if defined(CONFIG_MMU)
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
#ifdef CONFIG_CPU_MMU_V1
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
#ifdef CONFIG_CPU_MMU_V1
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

#ifdef CONFIG_CPU_MMU_V1
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
#ifdef CONFIG_CPU_MMU_V1
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
#ifdef CONFIG_CPU_MMU_V1
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
	lsri    a2, a1, _PGDIR_SHIFT
	ixw     a0, a2
	ldw     a0, (a0)
	
	zext    a1, a1, 21, 12
	ixw     a0, a1
	bclri   a0, PTE_BIT
	ldw     a1, (a0)
	WR_MEL0 a1
	ldw     a1, (a0, 4)
	WR_MEL1 a1
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
	bgeni   a2, 31      /* TLB probe command, a2 = 0x80000000 */
	
	WR_MCIR	a2          /* find faulting entry */
	movi    a2, _PAGE_WRITE
	and     a3, a2
	cmpnei  a3, 0
	bf      tlbmodified
	ldw     a3, (r6)
	
	/* 
	 * Present and writable bits set, set accessed and dirty bits. 
	 * a2 = (_PAGE_ACCESSED | _PAGE_MODIFIED | _PAGE_VALID | _PAGE_DIRTY)
	 */
#ifdef CONFIG_CPU_MMU_V1
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
#ifdef CONFIG_CPU_MMU_V1
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
	andn    a3, r8          /* r8 = !(0xfffff000) */
	mov     a2, a3
	psrset  ee, ie          /* Enable exception & interrupt */
	mov     a0, sp
	movi    a1, 1
	jbsr    (do_page_fault)
	movi    r11_sig, 0             /* r11 = 0, Not a syscall. */
	jmpi    (ret_from_exception)
#else
ENTRY(handle_tlbinvalidl)
ENTRY(handle_tlbinvalids)
ENTRY(handle_tlbmiss)
ENTRY(handle_tlbmodified)
	rte
#endif

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

#if defined(CONFIG_CPU_CSKYV1)
	addi    r13, 2                  /* Increase the epc */
#elif defined(CONFIG_CPU_CSKYV2)
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

	lrw     r11, NR_syscalls
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
#if defined(CONFIG_CPU_CSKYV2)
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

ENTRY(ret_from_exception)
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
	andi     r8, 7
	cmpnei   r8, 0
	bt       exit_work
1:  RESTORE_ALL
        
exit_work:
	lrw      a0, init_task          /* Address of init_task */
	ldw      a0, (a0)               /* Address of task_struct of task[0]*/
	cmpne    r10, a0
	bf       1b                     /* Exit if task[0] */

	mov      a0, sp                 /* Stack address is arg[0] */
	jbsr     set_esp0               /* Call C level */
	btsti    r8, TIF_NEED_RESCHED
	bt       work_resched
	cmpnei   r8, 0		/* check TIF_SIGPENDING and TIF_NOTIFY_RESUME*/
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
#if defined(CONFIG_CPU_CSKYV1)
	mtcr     sp, ss1
	mfcr     sp, ss0
	mtcr     a1, ss4
#elif defined(CONFIG_CPU_CSKYV2)
	subi     sp, 8
	stw      a1,(sp)
#endif
	mfcr     a1, epc                /* Get the trap point */
#if defined(CONFIG_CPU_CSKYV1)
	addi     a1, 2                  /* Increase the epc */
#elif defined(CONFIG_CPU_CSKYV2)
	addi     a1, 4                  /* Increase the epc, because the Instruct "trap x" in CK ISA V2 is 32 bit */
#endif
	mtcr     a1, epc                /* Save return point */
	
	movi     a1, 0x32
	mtcr     a1, cr17
#if defined(CONFIG_CPU_CSKYV1)	
	mfcr     a1, ss4
	mtcr     sp, ss0
	mfcr     sp, ss1
#elif defined(CONFIG_CPU_CSKYV2)
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
#if defined(CONFIG_CPU_CSKYV1)
	mtcr     sp, ss1
	mfcr     sp, ss0
#endif
	mfcr     a3, epc		/* Get the trap point */
#if defined(CONFIG_CPU_CSKYV1)
	addi     a3, 2			/* Increase the epc */
#elif defined(CONFIG_CPU_CSKYV2)
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
#if defined(CONFIG_CPU_CSKYV1)
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
#if defined(CONFIG_CPU_CSKYV1)
	mtcr     sp, ss1
	mfcr     sp, ss0
#endif

	subi     sp, 8                  /* because sp may align wich 0x2000 */
	mfcr     a0, epc                /* Get the trap point */
#if defined(CONFIG_CPU_CSKYV1)
	addi     a0, 2                  /* Increase the epc */
#elif defined(CONFIG_CPU_CSKYV2)
	addi     a0, 4                  /* Increase the epc, because the Instruct "trap x" in CK ISA V2 is 32 bit */
#endif
	mtcr     a0, epc                /* Save return point */
	
	bmaski   a0, (PAGE_SHIFT + 1)   /* kernel stack is 2*page if page is 4k */
	not      a0
	and      a0, sp                 /* thread_info local in bottom of stack */ 
	
	ldw      a0, (a0, TINFO_TP_VALURE) /* get tls */    
	
	addi     sp, 8  
#if defined(CONFIG_CPU_CSKYV1)	
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
#ifdef CONFIG_CPU_CSKYV1
	mfcr      a0,cr15
	bseti     a0, 28
	mtcr      a0, cr15     	       /* clear the exceptional state */
	cprcr     a0, cpcr4            /* read fesr to check the exception type */ 
#else
	mfcr      a0, cr<2, 2>	       /* fpu fesr is cr<2,2> in CSKY_CPUV2 */
#endif
	movi      r11_sig, 0           /* r11 = 0, Not a syscall. */
	mov       a1, sp               /* Push Stack pointer arg */
	jbsr      handle_fpe_c         /* Call C-level fpe handler */
	SET_SMOD_MMU_CP15
#endif
	jmpi      ret_from_exception

/*
 * handle interrupt.
 */
ENTRY(inthandler)
	SAVE_ALL
	SET_SMOD_MMU_CP15
	psrset   ee                     /* Enable exceptions */
	
	movi    r11_sig, 0                   /* r11 = 0, Not a syscall. */
	mov     r9, sp     					 /* Get current stack  pointer */
	bmaski  r10, THREADSIZE_MASK_BIT 
	andn    r9, r10                      /* Get thread_info */

#ifdef CONFIG_PREEMPT
	/*
	 * Get task_struct->stack.preempt_count for current, 
	 * and increase 1.
	 */
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
 * This is the fast interrupt handler (for certain hardware interrupt
 * sources). Unlike the normal interrupt handler it doesn't bother
 * doing the bottom half handlers.
 *
 */

#ifdef CONFIG_CPU_USE_FIQ
ENTRY(fasthandler)
	mfcr     a0, psr                /* Get PSR register */
	lsri     a0, 16                 /* Get vector in base 8 bits */
	sextb    a0                     /* Fill upper bytes with 0 */
	subi     a0, 32                  /* Real irq nomber need sub VEC offset(32)*/
	jbsr     csky_do_FIQ          /* Call handler */
	rfi 
#endif

/*
 * This is the fast aotu-vector interrupt handler (for certain hardware
 * interrupt sources). Unlike the normal interrupt handler it doesn't
 * bother doing the bottom half handlers.
 *
 */
ENTRY(fastautohandler)
#ifdef CONFIG_CPU_USE_FIQ
	jbsr     csky_do_auto_FIQ          /* Call handler */
#endif
	rfi 

ENTRY(sys_fork)
	PT_REGS_ADJUST a0	
	jmpi     csky_fork            /* Call fork routine */

ENTRY(sys_clone)
	/* pt_regs in arg 6 */
#if defined(__CSKYABIV2__)	
	addi     t0, sp, 8            
	stw      t0, (sp, 4)
#else
	mov      a5, sp             
#endif
	jmpi     csky_clone           /* Call clone routine */

ENTRY(sys_vfork)
	PT_REGS_ADJUST a0	
	jmpi     csky_vfork           /* Call fork routine */

ENTRY(sys_sigreturn)
	PT_REGS_ADJUST a0
	movi     r11_sig, 0             /* prevent syscall restart handling */
	jmpi     do_sigreturn

ENTRY(sys_rt_sigreturn)
	PT_REGS_ADJUST a0
	movi     r11_sig, 0             /* prevent syscall restart handling */
	jmpi     do_rt_sigreturn

ENTRY(sys_set_thread_area)
	PT_REGS_ADJUST a1
	jmpi     do_set_thread_area       /* Call set_thread_area routine */

/*
 * Resume execution of a new process.
 * Register definitions comming in:
 *
 * a0   =  current task
 * a1   =  new task
 */

ENTRY(resume)
	lrw      a3, TASK_THREAD        /* struct_thread offset in task_struct */ 
	addu     a3, a0                 /* a3 point to thread in task_struct */
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

	lrw      a3, TASK_THREAD
	addu     a3, a1                 /* Pointer to thread in task_struct */
	
	/* Set up next process to run */
	ldw      sp, (a3, THREAD_KSP)   /* Set next ksp */
	ldw      r6, (a3, THREAD_USP)   /* Set next usp */

#if defined(__CSKYABIV2__)
	mtcr     r6, cr<14, 1>           /* Get current usp */
#else
	mtcr     r6, ss1                /* Get current usp */
#endif

#ifdef CONFIG_CPU_HAS_FPU 
	FPU_RESTORE_REGS
#endif

#if  defined(CONFIG_CPU_HAS_DSP) || defined(__CK810__)
	lrw      r10, THREAD_DSPHI
	add      r10, a3 
	ldw      r6, (r10, 8)   /* THREAD_DSPCSR */
#if defined(__CK810__)
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
	
#if  defined(CONFIG_CPU_CSKYV2)
	/* set TLS register (r31) */
	addi     r7, a1, TASK_THREAD_INFO
	ldw      r31, (r7, TINFO_TP_VALURE)
#endif

	RESTORE_SWITCH_STACK	

	rts

.data
ALIGN
sys_call_table:
	.long sys_restart_syscall /* 0 old "setup" system call, used for restart*/
	.long sys_exit
	.long sys_fork
	.long sys_read
	.long sys_write
	.long sys_open          /* 5 */
	.long sys_close
	.long sys_waitpid
	.long sys_creat
	.long sys_link
	.long sys_unlink        /* 10 */
	.long sys_execve
	.long sys_chdir
	.long sys_time
	.long sys_mknod
	.long sys_chmod         /* 15 */
	.long sys_chown16
	.long sys_ni_syscall    /* old break syscall holder */
	.long sys_stat
	.long sys_lseek
	.long sys_getpid        /* 20 */
	.long sys_mount
	.long sys_oldumount
	.long sys_setuid16
	.long sys_getuid16
	.long sys_stime         /* 25 */
	.long sys_ptrace
	.long sys_alarm
	.long sys_fstat
	.long sys_pause
	.long sys_utime         /* 30 */
	.long sys_ni_syscall    /* old stty syscall holder */
	.long sys_ni_syscall    /* old gtty syscall holder */
	.long sys_access
	.long sys_nice
	.long sys_ni_syscall    /* 35 old ftime syscall holder */
	.long sys_sync
	.long sys_kill
	.long sys_rename
	.long sys_mkdir
	.long sys_rmdir         /* 40 */
	.long sys_dup
	.long sys_pipe
	.long sys_times
	.long sys_ni_syscall    /* old prof syscall holder */
	.long sys_brk           /* 45 */
	.long sys_setgid16
	.long sys_getgid16
	.long sys_signal
	.long sys_geteuid16
	.long sys_getegid16     /* 50 */
	.long sys_acct
	.long sys_umount        /* recycled never used phys() */
	.long sys_ni_syscall    /* old lock syscall holder */
	.long sys_ioctl
	.long sys_fcntl         /* 55 */
	.long sys_ni_syscall    /* old mpx syscall holder */
	.long sys_setpgid
	.long sys_ni_syscall    /* old ulimit syscall holder */
	.long sys_ni_syscall
	.long sys_umask         /* 60 */
	.long sys_chroot
	.long sys_ustat
	.long sys_dup2
	.long sys_getppid
	.long sys_getpgrp       /* 65 */
	.long sys_setsid
	.long sys_sigaction
	.long sys_sgetmask
	.long sys_ssetmask
	.long sys_setreuid16    /* 70 */
	.long sys_setregid16
	.long sys_sigsuspend
	.long sys_sigpending
	.long sys_sethostname   
	.long sys_setrlimit     /* 75 */
	.long sys_old_getrlimit
	.long sys_getrusage
	.long sys_gettimeofday
	.long sys_settimeofday  
	.long sys_getgroups16   /* 80 */
	.long sys_setgroups16
	.long old_select
	.long sys_symlink
	.long sys_lstat
	.long sys_readlink      /* 85 */
	.long sys_uselib
	.long sys_swapon
	.long sys_reboot
	.long sys_old_readdir
	.long old_mmap          /* 90 */
	.long sys_munmap
	.long sys_truncate
	.long sys_ftruncate
	.long sys_fchmod
	.long sys_fchown16      /* 95 */
	.long sys_getpriority
	.long sys_setpriority
	.long sys_ni_syscall    /* old profil syscall holder */
	.long sys_statfs
	.long sys_fstatfs       /* 100 */
	.long sys_ni_syscall    /* ioperm for i386 */
	.long sys_socketcall
	.long sys_syslog
	.long sys_setitimer     
	.long sys_getitimer     /* 105 */
	.long sys_newstat
	.long sys_newlstat
	.long sys_newfstat
	.long sys_ni_syscall    
	.long sys_ni_syscall    /* 110 iopl for i386 */
 	.long sys_vhangup
	.long sys_ni_syscall    /* obsolete idle() syscall */
	.long sys_ni_syscall    /* vm86old for i386 */
	.long sys_wait4
	.long sys_swapoff       /* 115 */
	.long sys_sysinfo
	.long sys_ipc
	.long sys_fsync
	.long sys_sigreturn
	.long sys_clone         /* 120 */
	.long sys_setdomainname
	.long sys_newuname
	.long sys_cacheflush    /* modify_ldt for i386 */
	.long sys_adjtimex
	.long sys_mprotect      /* 125 */
	.long sys_sigprocmask
	.long sys_ni_syscall    /* old "create_module" */
	.long sys_init_module
	.long sys_delete_module
	.long sys_ni_syscall    /* 130 - old "get_kernel_syms" */
	.long sys_quotactl
	.long sys_getpgid
	.long sys_fchdir
	.long sys_bdflush
	.long sys_sysfs         /* 135 */
	.long sys_personality
	.long sys_ni_syscall    /* for afs_syscall */
	.long sys_setfsuid16
	.long sys_setfsgid16
	.long sys_llseek        /* 140 */
	.long sys_getdents
	.long sys_select
	.long sys_flock
	.long sys_msync
	.long sys_readv         /* 145 */
	.long sys_writev
	.long sys_getsid
	.long sys_fdatasync
	.long sys_sysctl
	.long sys_mlock         /* 150 */
	.long sys_munlock
	.long sys_mlockall
	.long sys_munlockall
	.long sys_sched_setparam
	.long sys_sched_getparam        /* 155 */
	.long sys_sched_setscheduler
	.long sys_sched_getscheduler
	.long sys_sched_yield
	.long sys_sched_get_priority_max
	.long sys_sched_get_priority_min  /* 160 */
	.long sys_sched_rr_get_interval
	.long sys_nanosleep
	.long sys_mremap
	.long sys_setresuid16
	.long sys_getresuid16   /* 165 */
	.long sys_getpagesize
	.long sys_ni_syscall    /* old sys_query_module */
	.long sys_poll
	.long 0
	.long sys_setresgid16   /* 170 */
	.long sys_getresgid16
	.long sys_prctl
	.long sys_rt_sigreturn
	.long sys_rt_sigaction
	.long sys_rt_sigprocmask        /* 175 */
	.long sys_rt_sigpending
 	.long sys_rt_sigtimedwait
	.long sys_rt_sigqueueinfo
	.long sys_rt_sigsuspend
	.long sys_pread64               /* 180 */
	.long sys_pwrite64                
	.long sys_lchown16;
	.long sys_getcwd
	.long sys_capget
	.long sys_capset        /* 185 */
	.long sys_sigaltstack
	.long sys_sendfile
	.long sys_ni_syscall    /* streams1 */
	.long sys_ni_syscall    /* streams2 */
	.long sys_vfork         /* 190 */
	.long sys_getrlimit     
	.long sys_mmap2
	.long sys_truncate64
	.long sys_ftruncate64
	.long sys_stat64        /* 195 */
	.long sys_lstat64
	.long sys_fstat64
	.long sys_chown
	.long sys_getuid
	.long sys_getgid        /* 200 */
	.long sys_geteuid
	.long sys_getegid
	.long sys_setreuid
	.long sys_setregid
	.long sys_getgroups     /* 205 */
	.long sys_setgroups
	.long sys_fchown
	.long sys_setresuid
	.long sys_getresuid
	.long sys_setresgid     /* 210 */
	.long sys_getresgid
	.long sys_lchown
	.long sys_setuid
	.long sys_setgid
	.long sys_setfsuid      /* 215 */
	.long sys_setfsgid
	.long sys_pivot_root
	.long sys_set_thread_area
	.long sys_ni_syscall
	.long sys_getdents64    /* 220 */
	.long sys_gettid
	.long sys_tkill
	.long sys_setxattr
	.long sys_lsetxattr
	.long sys_fsetxattr     /* 225 */
	.long sys_getxattr
	.long sys_lgetxattr
	.long sys_fgetxattr
	.long sys_listxattr
	.long sys_llistxattr    /* 230 */
	.long sys_flistxattr
	.long sys_removexattr
	.long sys_lremovexattr
	.long sys_fremovexattr
	.long sys_futex         /* 235 */
	.long sys_sendfile64
	.long sys_mincore
	.long sys_madvise
	.long sys_fcntl64
	.long sys_readahead     /* 240 */
	.long sys_io_setup
	.long sys_io_destroy
	.long sys_io_getevents
	.long sys_io_submit
	.long sys_io_cancel     /* 245 */
	.long sys_fadvise64
	.long sys_exit_group
	.long sys_lookup_dcookie
	.long sys_epoll_create
	.long sys_epoll_ctl     /* 250 */
	.long sys_epoll_wait
	.long sys_remap_file_pages
	.long sys_set_tid_address
	.long sys_timer_create
	.long sys_timer_settime /* 255 */
	.long sys_timer_gettime
	.long sys_timer_getoverrun
	.long sys_timer_delete
	.long sys_clock_settime
	.long sys_clock_gettime /* 260 */
	.long sys_clock_getres
	.long sys_clock_nanosleep
	.long sys_statfs64
	.long sys_fstatfs64
	.long sys_tgkill        /* 265 */
	.long sys_utimes
	.long sys_csky_fadvise64_64
	.long sys_mbind
	.long sys_get_mempolicy
	.long sys_set_mempolicy /* 270 */
	.long sys_mq_open
	.long sys_mq_unlink
	.long sys_mq_timedsend
	.long sys_mq_timedreceive
	.long sys_mq_notify     /* 275 */
	.long sys_mq_getsetattr
	.long sys_waitid
	.long sys_ni_syscall    /* for sys_vserver */
	.long sys_add_key
	.long sys_request_key   /* 280 */
	.long sys_keyctl
	.long sys_ioprio_set
	.long sys_ioprio_get
	.long sys_inotify_init
	.long sys_inotify_add_watch     /* 285 */
	.long sys_inotify_rm_watch
	.long sys_migrate_pages
	.long sys_openat
	.long sys_mkdirat
	.long sys_mknodat               /* 290 */
	.long sys_fchownat
	.long sys_futimesat
	.long sys_fstatat64
	.long sys_unlinkat
	.long sys_renameat              /* 295 */
	.long sys_linkat
	.long sys_symlinkat
	.long sys_readlinkat
	.long sys_fchmodat
	.long sys_faccessat             /* 300 */
	.long sys_pselect6
	.long sys_ppoll
	.long sys_unshare
	.long sys_set_robust_list
	.long sys_get_robust_list       /* 305 */
	.long sys_splice
	.long sys_sync_file_range2
	.long sys_tee
 	.long sys_vmsplice
	.long sys_move_pages            /* 310 */
	.long sys_sched_setaffinity
	.long sys_sched_getaffinity
	.long sys_kexec_load
	.long sys_getcpu
	.long sys_epoll_pwait           /* 315 */
	.long sys_utimensat     
	.long sys_signalfd
	.long sys_timerfd_create
	.long sys_eventfd
	.long sys_fallocate             /* 320 */
	.long sys_timerfd_settime
	.long sys_timerfd_gettime
	.long sys_signalfd4
	.long sys_eventfd2
	.long sys_epoll_create1         /* 325 */
	.long sys_dup3
	.long sys_pipe2
	.long sys_inotify_init1
	.long sys_preadv
	.long sys_pwritev               /* 330 */
	.long sys_rt_tgsigqueueinfo
	.long sys_perf_event_open
	.long sys_recvmmsg
	.long sys_accept4 
	.long sys_fanotify_init         /* 335 */
	.long sys_fanotify_mark
	.long sys_prlimit64
	.long sys_name_to_handle_at
	.long sys_open_by_handle_at
	.long sys_clock_adjtime         /* 340 */
	.long sys_syncfs
	.long sys_sendmmsg
	.long sys_setns
	.long csky_prfl_trig            //new systemcall
	.long csky_prfl_read            /* 345*/

