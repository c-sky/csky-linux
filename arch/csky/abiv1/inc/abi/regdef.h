// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef  __ASM_CSKY_REGDEF_H
#define  __ASM_CSKY_REGDEF_H

#define syscallid	r1
#define r11_sig		r11

#define DEFAULT_PSR_VALUE	0x8f000000

#define PTRACE_REGOFF_ABI \
{ \
	-1,       PT_REGS9,  PT_A0,    PT_A1,\
	PT_A2,    PT_A3,     PT_REGS0, PT_REGS1,\
	PT_REGS2, PT_REGS3,  PT_REGS4, PT_REGS5,\
	PT_REGS6, PT_REGS7,  PT_REGS8, PT_R15,\
	-1,       -1,        -1,       -1,\
	-1,       -1,        -1,       -1,\
	-1,       -1,        -1,       -1,\
	-1,       -1,        -1,       -1,\
	PT_SR,    PT_PC,     -1,       -1,\
}

#define SYSTRACE_SAVENUM	2
#define REGNO_USP		0

#endif /* __ASM_CSKY_REGDEF_H */
