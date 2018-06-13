// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef  __ASM_CSKY_REGDEF_H
#define  __ASM_CSKY_REGDEF_H

#define syscallid	r7
#define r11_sig		r11

#define regs_syscallid(regs) regs->regs[3]

#define DEFAULT_PSR_VALUE	0x8f000200

#define SYSTRACE_SAVENUM	5

#endif /* __ASM_CSKY_REGDEF_H */
