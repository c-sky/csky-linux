#ifndef __ASM_CSKY_ENTRY_H
#define __ASM_CSKY_ENTRY_H

#include <asm/setup.h>
#include <hal/regdef.h>

/*
 * Stack layout in 'ret_from_exception':
 *      Below describes the stack layout after initial exception entry.
 *      All traps, interrupts and exceptions will set up the stack frame
 *      in this way before starting processing proper.
 *	This allows access to the syscall arguments in registers r1-r5
 *
 *	 0(sp) - pc
 *	 4(sp) - orig_a0
 *	 8(sp) - sr
 *	 C(sp) - a0
 *	10(sp) - a1
 *	14(sp) - a2
 *	18(sp) - a3
 *	1C(sp) - regs0
 *	20(sp) - regs1
 *	24(sp) - regs2
 *	28(sp) - regs3
 *	2C(sp) - regs4
 *	30(sp) - regs5
 *	34(sp) - regs6
 *	38(sp) - regs7
 *	3C(sp) - regs8
 *	40(sp) - regs9
 *	44(sp) - r15
 */
#define LSAVE_A0       0xc
#define LSAVE_A1       0x10
#define LSAVE_A2       0x14
#define LSAVE_A3       0x18
#define LSAVE_REGS0    0x1C
#define LSAVE_REGS1    0x20

/*
 *      This code creates the normal kernel pt_regs layout on a trap
 *      or interrupt. The only trick here is that we check whether we
 *      came from supervisor mode before changing stack pointers.
 */

.macro	SAVE_ALL
        mtcr    regs7, ss2    /* save original regs7=r13 */
        mtcr    a0, ss3/* save original a0 */
        mfcr    regs7, epsr   /* Get original PSR */
        btsti   regs7, 31     /* Check if was supervisor */
        bt      1f
        mtcr    sp, ss1/* save user stack */
        mfcr    sp, ss0/* Set kernel stack */

1:
        subi    sp, 32
        subi    sp, 32
        stw     regs7, (sp, 0)  /* original epsr */
        stw     a0, (sp, 4)
        stw     a1, (sp, 8)
        stw     a2, (sp, 12)
        stw     a3, (sp, 16)
        stw     regs0, (sp, 20)
        stw     regs1, (sp, 24)
        stw     regs2, (sp, 28)
        stw     regs3, (sp, 32)
	stw     regs4, (sp, 36)
        stw     regs5, (sp, 40)
        stw     regs6, (sp, 44)
        mfcr    regs7, ss2     /* Save original regs7=r13 on stack */
        stw     regs7, (sp, 48)
        stw     regs8, (sp, 52)
        stw     regs9, (sp, 56)
        stw     r15, (sp, 60)

	subi    sp, 8        /* Make room for PC/orig_a0 */
	stw     a0, (sp, 4)      /*Save syscall a0 on stack */
        mfcr    regs7, epc     /* Save PC on stack */
        stw     regs7, (sp)
.endm

.macro	RESTORE_ALL
        psrclr  ie     /* Disable interrupt */
	ldw     a0, (sp)        /* Restore PC */
        mtcr    a0, epc/* Set return PC */
        ldw     a0, (sp, 8)     /* Get saved PSR */
        mtcr    a0, epsr        /* Restore PSR */
	addi    sp, 12
        btsti   a0, 31 /* Check if returning to user */
        ldw     a0, (sp, 0)
        ldw     a1, (sp, 4)
        ldw     a2, (sp, 8)
        ldw     a3, (sp, 12)
        ldw     regs0, (sp, 16)
        ldw     regs1, (sp, 20)
        ldw     regs2, (sp, 24)
        ldw     regs3, (sp, 28)
        ldw     regs4, (sp, 32)
        ldw     regs5, (sp, 36)
        ldw     regs6, (sp, 40)
        ldw     regs7, (sp, 44)
	ldw     regs8, (sp, 48)
        ldw     regs9, (sp, 52)
        ldw     r15, (sp, 56)
        addi    sp, 32 /* Increment stack pointer */
        addi    sp, 28
        bt      1f
        mtcr    sp, ss0/* Save kernel stack*/
        mfcr    sp, ss1/* Set  user stack */
1:
        rte
.endm

#define SAVE_SWITCH_STACK save_switch_stack
#define RESTORE_SWITCH_STACK restore_switch_stack
#define GET_CURRENT(tmp)

.macro	save_switch_stack
	subi    sp, 32
        stm     r8-r15,(sp)
.endm

.macro	restore_switch_stack
        ldm     r8-r15,(sp)
        addi    sp, 32
.endm

.macro  PT_REGS_ADJUST  rx   /* abiv2 when argc>5 need push r4 r5 in syscall */
        mov      \rx, sp
.endm
/*
 * Because kernel don't use FPU and only user program use FPU, we select
 * coprocessor 15(MMU) when in super-mode. So this macro is called when
 * CPU enter from user-mode to kernel super-mode except MMU exception.
 */
.macro SET_SMOD_MMU_CP15
	cpseti  cp15
.endm

/*
 * Below, are macros for MMU operating, use them to switch cop, read or write
 * registers of MMU in assemble files. Macro __CSKYABIV1__ means MMU in
 * coprocessor.
 */
/* Coprocessor switch to MMU */
.macro SET_CP_MMU
	cpseti  cp15
.endm

/* MMU registers read operators. */
.macro RD_MIR	rx
	cprcr   \rx, cpcr0
.endm

.macro RD_MRR	rx
	cprcr   \rx, cpcr1
.endm

.macro RD_MEL0	rx
	cprcr   \rx, cpcr2
.endm

.macro RD_MEL1	rx
	cprcr   \rx, cpcr3
.endm

.macro RD_MEH	rx
	cprcr   \rx, cpcr4
.endm

.macro RD_MCR	rx
	cprcr   \rx, cpcr5
.endm

.macro RD_MPR	rx
	cprcr   \rx, cpcr6
.endm

.macro RD_MWR	rx
	cprcr   \rx, cpcr7
.endm

.macro RD_MCIR	rx
	cprcr   \rx, cpcr8
.endm

.macro RD_PGDR  rx
        cprcr   \rx, cpcr29
.endm

/* MMU registers write operators. */
.macro WR_MIR	rx
	cpwcr   \rx, cpcr0
.endm

.macro WR_MRR	rx
	cpwcr   \rx, cpcr1
.endm

.macro WR_MEL0	rx
	cpwcr   \rx, cpcr2
.endm

.macro WR_MEL1	rx
	cpwcr   \rx, cpcr3
.endm

.macro WR_MEH	rx
	cpwcr   \rx, cpcr4
.endm

.macro WR_MCR	rx
	cpwcr   \rx, cpcr5
.endm

.macro WR_MPR	rx
	cpwcr   \rx, cpcr6
.endm

.macro WR_MWR	rx
	cpwcr   \rx, cpcr7
.endm

.macro WR_MCIR	rx
	cpwcr   \rx, cpcr8
.endm

.macro WR_MSA0	rx
	cpwcr   \rx, cpcr30
.endm

.macro WR_MSA1	rx
	cpwcr   \rx, cpcr31
.endm

#endif /* __ASM_CSKY_ENTRY_H */
