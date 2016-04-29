#ifndef __ASM_CSKY_ENTRY_H
#define __ASM_CSKY_ENTRY_H

#include <asm/setup.h>
#include <asm/regdef.h>

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

/*
 *      Offsets into the stack to get at save registers.
 */
#define LSAVE_SYSCALLR10x4
#define LSAVE_A0       0xc    //12
#define LSAVE_A1       0x10
#define LSAVE_A2       0x14
#define LSAVE_A3       0x18
#define LSAVE_REGS0    0x1C
#define LSAVE_REGS1    0x20
#define LSAVE_REGS2    0x24
#define LSAVE_REGS3    0x28
#define LSAVE_REGS4    0x2C
#define LSAVE_REGS5    0x30
#define LSAVE_REGS6    0x34
#define LSAVE_REGS7    0x38
#define LSAVE_REGS8    0x3C
#define LSAVE_REGS9    0x40
#define LSAVE_R15      0x44
#if defined(CONFIG_CPU_CSKYV2)
#define LSAVE_R16      0x48
#define LSAVE_R17      0x4C
#define LSAVE_R18      0x50
#define LSAVE_R19      0x54
#define LSAVE_R20      0x58
#define LSAVE_R21      0x5C
#define LSAVE_R22      0x60
#define LSAVE_R23      0x64
#define LSAVE_R24      0x68
#define LSAVE_R25      0x6C
#define LSAVE_R26      0x70
#define LSAVE_R27      0x74
#define LSAVE_R28      0x78
#define LSAVE_R29      0x7C
#define LSAVE_R30      0x80
#define LSAVE_R31      0x84
#define LSAVE_Rhi      0x88
#define LSAVE_Rlo      0x8C
#endif

#ifdef __ASSEMBLY__


/*
 *      This code creates the normal kernel pt_regs layout on a trap
 *      or interrupt. The only trick here is that we check whether we
 *      came from supervisor mode before changing stack pointers.
 */

.macro	SAVE_ALL 
#if defined(CONFIG_CPU_CSKYV1)
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
#else
    #if !defined(__CSKYABIV2__)
        mtcr    regs7, ss2/* save original r13*/
        mtcr    a0, ss3     /* save original a0 */
        mfcr    regs7,epsr/* Get original PSR */
        btsti   regs7, 31/* Check if was supervisor */
        bt      1f

	mtcr    sp, ss1  /* save user stack */
        mfcr    sp, ss0  /* Set kernel stack */

1:
	mfcr    regs7, ss2/* restore original r13*/
    #endif

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
#endif
.endm

.macro	RESTORE_ALL
        psrclr  ie     /* Disable interrupt */
	ldw     a0, (sp)        /* Restore PC */
        mtcr    a0, epc/* Set return PC */
        ldw     a0, (sp, 8)     /* Get saved PSR */
        mtcr    a0, epsr        /* Restore PSR */
	addi    sp, 12
#if defined(CONFIG_CPU_CSKYV1)
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
#else
    #if !defined(__CSKYABIV2__)
        btsti     a0, 31        /* Check if returning to kernel */
    #endif
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
    #if !defined (__CSKYABIV2__)
        bt      1f
        mtcr    sp, ss0/* Save kernel stack*/
        mfcr    sp, ss1/* Set  user stack */
    #endif
#endif
1:
        rte
.endm

#define SAVE_SWITCH_STACK save_switch_stack
#define RESTORE_SWITCH_STACK restore_switch_stack
#define GET_CURRENT(tmp)

.macro	save_switch_stack
#if defined(CONFIG_CPU_CSKYV1)
	subi    sp, 32
        stm     r8-r15,(sp)
#elif defined(CONFIG_CPU_CSKYV2) && !defined(__CSKYABIV2__)
        subi    sp, 68
        stm     r8-r19,(sp)
        stw     r26, (sp, 48)
        stw     r27, (sp, 52)
        stw     r28, (sp, 56)
        stw     r29, (sp, 60)
        stw     r30, (sp, 64)
#elif defined (__CSKYABIV2__)
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
#endif
.endm

.macro	restore_switch_stack
#if defined(CONFIG_CPU_CSKYV1)
        ldm     r8-r15,(sp)
        addi    sp, 32
#elif defined(CONFIG_CPU_CSKYV2) && !defined(__CSKYABIV2__)
        ldm     r8-r19,(sp)
        ldw     r26, (sp, 48)
        ldw     r27, (sp, 52)
        ldw     r28, (sp, 56)
        ldw     r29, (sp, 60)
        ldw     r30, (sp, 64)
        addi    sp, 68
#elif defined (__CSKYABIV2__)
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
#endif
.endm

.macro  PT_REGS_ADJUST  rx   /* abiv2 when argc>5 need push r4 r5 in syscall */
#if defined(__CSKYABIV2__)
        addi     \rx, sp, 8
#else
        mov      \rx, sp
#endif
.endm
/*
 * Because kernel don't use FPU and only user program use FPU, we select 
 * coprocessor 15(MMU) when in super-mode. So this macro is called when 
 * CPU enter from user-mode to kernel super-mode except MMU exception.
 */
.macro SET_SMOD_MMU_CP15
#if defined(CONFIG_CPU_HAS_FPU) && defined(CONFIG_CPU_CSKYV1)
    cpseti  cp15
#endif
.endm

/*
 * Below, are macros for MMU operating, use them to switch cop, read or write
 * registers of MMU in assemble files. Macro CONFIG_CPU_CSKYV1 means MMU in
 * coprocessor.
 */
/* Coprocessor switch to MMU */
.macro SET_CP_MMU
#ifdef CONFIG_CPU_CSKYV1
	cpseti  cp15
#endif
.endm

/* MMU registers read operators. */
.macro RD_MIR	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr0
#else
	mfcr    \rx, cr<0, 15>
#endif
.endm

.macro RD_MRR	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr1
#else
	mfcr    \rx, cr<1, 15>
#endif
.endm

.macro RD_MEL0	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr2
#else
	mfcr    \rx, cr<2, 15>
#endif
.endm

.macro RD_MEL1	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr3
#else
	mfcr    \rx, cr<3, 15>
#endif
.endm

.macro RD_MEH	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr4
#else
	mfcr    \rx, cr<4, 15>
#endif
.endm

.macro RD_MCR	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr5
#else
	mfcr    \rx, cr<5, 15>
#endif
.endm

.macro RD_MPR	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr6
#else
	mfcr    \rx, cr<6, 15>
#endif
.endm

.macro RD_MWR	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr7
#else
	mfcr    \rx, cr<7, 15>
#endif
.endm

.macro RD_MCIR	rx
#ifdef CONFIG_CPU_CSKYV1 
	cprcr   \rx, cpcr8
#else
	mfcr    \rx, cr<8, 15>
#endif
.endm

.macro RD_PGDR  rx
#ifdef CONFIG_CPU_CSKYV1 
        cprcr   \rx, cpcr29
#else
        mfcr    \rx, cr<29, 15>
#endif
.endm

/* MMU registers write operators. */
.macro WR_MIR	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr0
#else
	mtcr    \rx, cr<0, 15>
#endif
.endm

.macro WR_MRR	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr1
#else
	mtcr    \rx, cr<1, 15>
#endif
.endm

.macro WR_MEL0	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr2
#else
	mtcr    \rx, cr<2, 15>
#endif
.endm
.macro WR_MEL1	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr3
#else
	mtcr    \rx, cr<3, 15>
#endif
.endm

.macro WR_MEH	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr4
#else
	mtcr    \rx, cr<4, 15>
#endif
.endm

.macro WR_MCR	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr5
#else
	mtcr    \rx, cr<5, 15>
#endif
.endm

.macro WR_MPR	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr6
#else
	mtcr    \rx, cr<6, 15>
#endif
.endm

.macro WR_MWR	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr7
#else
	mtcr    \rx, cr<7, 15>
#endif
.endm

.macro WR_MCIR	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr8
#else
	mtcr    \rx, cr<8, 15>
#endif
.endm

.macro WR_MSA0	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr30
#else
	mtcr    \rx, cr<30, 15>
#endif
.endm

.macro WR_MSA1	rx
#ifdef CONFIG_CPU_CSKYV1
	cpwcr   \rx, cpcr31
#else
	mtcr    \rx, cr<31, 15>
#endif
.endm

#else /* C source */

#define STR(X) STR1(X)
#define STR1(X) #X

#define PT_OFF_ORIG_D0	 0x24
#define PT_OFF_FORMATVEC 0x32
#define PT_OFF_SR	 0x2C

#endif

#endif /* __ASM_CSKY_ENTRY_H */
