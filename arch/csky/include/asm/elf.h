// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASMCSKY_ELF_H
#define __ASMCSKY_ELF_H

/*
 * ELF register definitions..
 */

#include <asm/ptrace.h>
#include <asm/user.h>
#include <abi/regdef.h>

#define ELF_ARCH 39

/* CSKY Relocations */
#define R_CSKY_NONE               0
#define R_CSKY_32                 1
#define R_CSKY_PCIMM8BY4          2
#define R_CSKY_PCIMM11BY2         3
#define R_CSKY_PCIMM4BY2          4
#define R_CSKY_PC32               5
#define R_CSKY_PCRELJSR_IMM11BY2  6
#define R_CSKY_GNU_VTINHERIT      7
#define R_CSKY_GNU_VTENTRY        8
#define R_CSKY_RELATIVE           9
#define R_CSKY_COPY               10
#define R_CSKY_GLOB_DAT           11
#define R_CSKY_JUMP_SLOT          12
#define R_CSKY_ADDR_HI16          24
#define R_CSKY_ADDR_LO16          25
#define R_CSKY_PCRELJSR_IMM26BY2  40

typedef unsigned long elf_greg_t;


#define ELF_NGREG (sizeof(struct pt_regs) / sizeof(elf_greg_t))

typedef elf_greg_t elf_gregset_t[ELF_NGREG];

typedef struct user_cskyfp_struct elf_fpregset_t;

/*
 * This is used to ensure we don't load something for the wrong architecture.
 */
#define elf_check_arch(x) ((x)->e_machine == ELF_ARCH)

/*
 * These are used to set parameters in the core dumps.
 */
#define USE_ELF_CORE_DUMP
#define ELF_EXEC_PAGESIZE		4096
#define ELF_CLASS			ELFCLASS32
#define ELF_PLAT_INIT(_r, load_addr)	_r->a0 = 0

#ifdef  __cskyBE__
#define ELF_DATA	ELFDATA2MSB
#else
#define ELF_DATA	ELFDATA2LSB
#endif

/* This is the location that an ET_DYN program is loaded if exec'ed.  Typical
   use of this is to invoke "./ld.so someprog" to test out a new version of
   the loader.  We need to make sure that it is out of the way of the program
   that it will "exec", and that there is sufficient room for the brk.  */

#define ELF_ET_DYN_BASE	0x0UL

/* The member sort in array pr_reg[x] is pc, r1, r0, psr, r2, r3,r4,
   r5, r6...... Because GDB difine */
#if defined(__CSKYABIV2__)
   #define ELF_CORE_COPY_REGS(pr_reg, regs)     \
        pr_reg[0] = regs->pc;                   \
        pr_reg[1] = regs->a1;                   \
        pr_reg[2] = regs->a0;                   \
        pr_reg[3] = regs->sr;                   \
        pr_reg[4] = regs->a2;                   \
        pr_reg[5] = regs->a3;                   \
        pr_reg[6] = regs->regs[0];              \
        pr_reg[7] = regs->regs[1];              \
        pr_reg[8] = regs->regs[2];              \
        pr_reg[9] = regs->regs[3];              \
        pr_reg[10] = regs->regs[4];             \
        pr_reg[11] = regs->regs[5];             \
        pr_reg[12] = regs->regs[6];             \
        pr_reg[13] = regs->regs[7];             \
        pr_reg[14] = regs->regs[8];             \
        pr_reg[15] = regs->regs[9];             \
        pr_reg[16] = rdusp();		        \
        pr_reg[17] = regs->r15;                 \
        pr_reg[18] = regs->exregs[0];           \
        pr_reg[19] = regs->exregs[1];           \
        pr_reg[20] = regs->exregs[2];           \
        pr_reg[21] = regs->exregs[3];           \
        pr_reg[22] = regs->exregs[4];           \
        pr_reg[23] = regs->exregs[5];           \
        pr_reg[24] = regs->exregs[6];           \
        pr_reg[25] = regs->exregs[7];           \
        pr_reg[26] = regs->exregs[8];           \
        pr_reg[27] = regs->exregs[9];           \
        pr_reg[28] = regs->exregs[10];          \
        pr_reg[29] = regs->exregs[11];          \
        pr_reg[30] = regs->exregs[12];          \
        pr_reg[31] = regs->exregs[13];          \
        pr_reg[32] = regs->exregs[14];          \
        pr_reg[33] = regs->exregs[15];
#else
     #define ELF_CORE_COPY_REGS(pr_reg, regs)   \
        pr_reg[0] = regs->pc;                   \
        pr_reg[1] = regs->regs[9];              \
        pr_reg[2] = rdusp();                    \
        pr_reg[3] = regs->sr;                   \
        pr_reg[4] = regs->a0;                   \
        pr_reg[5] = regs->a1;                   \
        pr_reg[6] = regs->a2;                   \
        pr_reg[7] = regs->a3;                   \
        pr_reg[8] = regs->regs[0];              \
        pr_reg[9] = regs->regs[1];              \
        pr_reg[10] = regs->regs[2];             \
        pr_reg[11] = regs->regs[3];             \
        pr_reg[12] = regs->regs[4];             \
        pr_reg[13] = regs->regs[5];             \
        pr_reg[14] = regs->regs[6];             \
        pr_reg[15] = regs->regs[7];             \
        pr_reg[16] = regs->regs[8];             \
        pr_reg[17] = regs->r15;
#endif

/* Similar, but for a thread other than current. */
struct task_struct;
extern int dump_task_regs(struct task_struct *, elf_gregset_t *);
#define ELF_CORE_COPY_TASK_REGS(tsk, elf_regs) dump_task_regs(tsk, elf_regs)

/* This yields a mask that user programs can use to figure out what
   instruction set this cpu supports.  */

#define ELF_HWCAP	(0)

/* This yields a string that ld.so will use to load implementation
   specific libraries for optimization.  This is more specific in
   intent than poking at uname or /proc/cpuinfo.  */

#define ELF_PLATFORM	(NULL)
#define SET_PERSONALITY(ex) set_personality(PER_LINUX)
#define ARCH_HAS_SETUP_ADDITIONAL_PAGES 1
struct linux_binprm;
extern int arch_setup_additional_pages(
		struct linux_binprm *bprm,
		int uses_interp);

#endif
