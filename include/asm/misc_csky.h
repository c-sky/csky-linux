/*
 * linux/arch/csky/include/asm/misc_abiv1.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */

#ifndef __CSKY_MISC_H__
#define __CSKY_MISC_H__
#include <asm/ptrace.h>
inline unsigned int read_pt_regs(unsigned int rx, struct pt_regs *regs);
inline void write_pt_regs(unsigned int value, unsigned int rx, struct pt_regs *regs);
inline unsigned int read_fpcr(void);
inline void write_fpcr(unsigned int val);
inline unsigned int read_fpesr(void);
inline void write_fpesr(unsigned int val);
#ifdef CONFIG_CPU_CSKYV1
inline unsigned int read_fpsr(void);
inline void write_fpsr(unsigned int val);
#endif
#endif /* __CSKY_MISC_H__ */
