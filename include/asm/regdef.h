/*
 * arch/csky/include/asm/regdef.h
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file COPYING in the main directory of
 * this archive for more details.
 *
 * (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  
 */

#ifndef  _CSKY_REGDEF_H
#define  _CSKY_REGDEF_H

#ifndef __CSKYABIV2__
#define  sp           r0   /* stack pointer */
#define  syscallid    r1   
#define  a0           r2   /*argument word 1 or function return value */
#define  a1           r3   /*argument word 2 or function return value */
#define  a2           r4   /*argument word 3-6 */
#define  a3           r5
#define  regs0        r6
#define  regs1        r7
#define  regs2        r8
#define  regs3        r9
#define  regs4        r10
#define  regs5        r11
#define  regs6        r12
#define  regs7        r13
#define  regs8        r14
#define  regs9        r1
#else                    /* __CSKYABIV2__ */
#define  sp           r14  /* stack pointer */
#define  syscallid    r7
#define  a0           r0   /*argument word 1 or function return value */
#define  a1           r1   /*argument word 2 or function return value */
#define  a2           r2   /*argument word 3-4 */
#define  a3           r3
#define  regs0        r4
#define  regs1        r5
#define  regs2        r6
#define  regs3        r7
#define  regs4        r8
#define  regs5        r9
#define  regs6        r10
#define  regs7        r11
#define  regs8        r12
#define  regs9        r13
#endif 

#define  r11_sig      r11 /* use as judge restart syscall */

#endif /* _CSKY_REGDEF_H */
