// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef  __ASM_CSKY_REGDEF_H
#define  __ASM_CSKY_REGDEF_H

#define syscallid	r7
#define r11_sig		r11

#define DEFAULT_PSR_VALUE	0x8f000200

#define PTRACE_REGOFF_ABI \
{ \
	PT_A0,    PT_A1,     PT_A2,    PT_A3,\
	PT_REGS0, PT_REGS1,  PT_REGS2, PT_REGS3,\
	PT_REGS4, PT_REGS5,  PT_REGS6, PT_REGS7,\
	PT_REGS8, PT_REGS9,  -1,       PT_R15,\
	PT_R16,   PT_R17,    PT_R18,   PT_R19,\
	PT_R20,   PT_R21,    PT_R22,   PT_R23,\
	PT_R24,   PT_R25,    PT_R26,   PT_R27,\
	PT_R28,   PT_R29,    PT_R30,   PT_R31,\
	PT_SR,    PT_PC,     PT_RHI,   PT_RLO,\
}

#define SYSTRACE_SAVENUM	5

#define REGNO_USP		14

#endif /* __ASM_CSKY_REGDEF_H */
