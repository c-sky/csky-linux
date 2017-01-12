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
	subi    sp,  144
        stw     a0, (sp, 4)
        stw     a0, (sp, 12)
        stw     a1, (sp, 16)
        stw     a2, (sp, 20)
        stw     a3, (sp, 24)
        stw     regs0, (sp, 28)
        stw     regs1, (sp, 32)
        stw     regs2, (sp, 36)
        stw     regs3, (sp, 40)
        stw     regs4, (sp, 44)
        stw     regs5, (sp, 48)
        stw     regs6, (sp, 52)
        stw     regs7, (sp, 56)
        stw     regs8, (sp, 60)
        stw     regs9, (sp, 64)
        stw     r15, (sp, 68)
        addi    sp, 72
        stm     r16-r31,(sp)
        mfhi    r22
        mflo    r23
	stw     r22, (sp, 64)
        stw     r23, (sp, 68)
        subi    sp,  72

	mfcr    r22, epsr        /* Get original PSR */
        stw     r22, (sp, 8)     /* Save psr on stack */
        mfcr    r22, epc/* Save PC on stack */
        stw     r22, (sp)
.endm

.macro	RESTORE_ALL
        psrclr  ie     /* Disable interrupt */
	ldw     a0, (sp)        /* Restore PC */
        mtcr    a0, epc/* Set return PC */
        ldw     a0, (sp, 8)     /* Get saved PSR */
        mtcr    a0, epsr        /* Restore PSR */
	addi    sp, 12
	ldw     a0, (sp, 124)
        ldw     a1, (sp, 128)
	mthi    a0
        mtlo    a1

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
        addi    sp, 60 /* Increment stack pointer */
        ldm     r16-r31,(sp)
        addi    sp,  72
1:
        rte
.endm

#define SAVE_SWITCH_STACK save_switch_stack
#define RESTORE_SWITCH_STACK restore_switch_stack
#define GET_CURRENT(tmp)

.macro	save_switch_stack
        subi    sp, 64
        stm     r4-r11,(sp)
        stw     r15, (sp, 32)
        stw     r16, (sp, 36)
        stw     r17, (sp, 40)
        stw     r26, (sp, 44)
        stw     r27, (sp, 48)
        stw     r28, (sp, 52)
        stw     r29, (sp, 56)
        stw     r30, (sp, 60)
.endm

.macro	restore_switch_stack
        ldm     r4-r11,(sp)
        ldw     r15, (sp, 32)
        ldw     r16, (sp, 36)
        ldw     r17, (sp, 40)
        ldw     r26, (sp, 44)
        ldw     r27, (sp, 48)
        ldw     r28, (sp, 52)
        ldw     r29, (sp, 56)
        ldw     r30, (sp, 60)
        addi    sp, 64
.endm

.macro  PT_REGS_ADJUST  rx   /* abiv2 when argc>5 need push r4 r5 in syscall */
        addi     \rx, sp, 8
.endm
/*
 * Because kernel don't use FPU and only user program use FPU, we select
 * coprocessor 15(MMU) when in super-mode. So this macro is called when
 * CPU enter from user-mode to kernel super-mode except MMU exception.
 */
.macro SET_SMOD_MMU_CP15
.endm

/*
 * Below, are macros for MMU operating, use them to switch cop, read or write
 * registers of MMU in assemble files. Macro __CSKYABIV1__ means MMU in
 * coprocessor.
 */
/* Coprocessor switch to MMU */
.macro SET_CP_MMU
.endm

/* MMU registers read operators. */
.macro RD_MIR	rx
	mfcr    \rx, cr<0, 15>
.endm

.macro RD_MRR	rx
	mfcr    \rx, cr<1, 15>
.endm

.macro RD_MEL0	rx
	mfcr    \rx, cr<2, 15>
.endm

.macro RD_MEL1	rx
	mfcr    \rx, cr<3, 15>
.endm

.macro RD_MEH	rx
	mfcr    \rx, cr<4, 15>
.endm

.macro RD_MCR	rx
	mfcr    \rx, cr<5, 15>
.endm

.macro RD_MPR	rx
	mfcr    \rx, cr<6, 15>
.endm

.macro RD_MWR	rx
	mfcr    \rx, cr<7, 15>
.endm

.macro RD_MCIR	rx
	mfcr    \rx, cr<8, 15>
.endm

.macro RD_PGDR  rx
        mfcr    \rx, cr<29, 15>
.endm

/* MMU registers write operators. */
.macro WR_MIR	rx
	mtcr    \rx, cr<0, 15>
.endm

.macro WR_MRR	rx
	mtcr    \rx, cr<1, 15>
.endm

.macro WR_MEL0	rx
	mtcr    \rx, cr<2, 15>
.endm
.macro WR_MEL1	rx
	mtcr    \rx, cr<3, 15>
.endm

.macro WR_MEH	rx
	mtcr    \rx, cr<4, 15>
.endm

.macro WR_MCR	rx
	mtcr    \rx, cr<5, 15>
.endm

.macro WR_MPR	rx
	mtcr    \rx, cr<6, 15>
.endm

.macro WR_MWR	rx
	mtcr    \rx, cr<7, 15>
.endm

.macro WR_MCIR	rx
	mtcr    \rx, cr<8, 15>
.endm

.macro WR_MSA0	rx
	mtcr    \rx, cr<30, 15>
.endm

.macro WR_MSA1	rx
	mtcr    \rx, cr<31, 15>
.endm

#endif /* __ASM_CSKY_ENTRY_H */
