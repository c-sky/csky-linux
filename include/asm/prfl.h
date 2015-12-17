/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2013  Hangzhou C-SKY Microsystems co.,ltd.
 */

#ifndef __CSKY_PRFL_H
#define __CSKY_PRFL_H

/* Define the state of 	HPCR control reg */
#define  HPCR_ECR    (1 << 31)   /* ECR=1,event count=0 */
#define  HPCR_HPCCR  (1 << 30)   /* HPCCR=1, hardware profiling count=0 */
#define  HPCR_TS     (1 << 29)   /* TS=1, start count */
#define  HPCR_TCE    (1 << 4)    /* trig enable bit */
#define  HPCR_UCE    (1 << 3)    /* USE=1,event count in USE mode */
#define  HPCR_SCE    (1 << 2)    /* SCE=1,event count in SUPER mode */
#define  HPCR_HPCCE  (1 << 1)    /* HPCCE=1,totle count enable bit */
#define  HPCR_ECE    (1 << 0)    /* ECE=1,event count enable bit*/

#ifdef __ASSEMBLY__

#include <asm/asm-offsets.h>

.macro PRFL_SAVE_REGS
        cprcr  r6, <0, 0x0>
        cprcr  r7, <0, 0x1>
        cprcr  r8, <0, 0x2>
        cprcr  r9, <0, 0x3>
        stw    r6, (a3, THREAD_HPCR)
        stw    r7, (a3, THREAD_HPSPR)
        stw    r8, (a3, THREAD_HPEPR)
        stw    r9, (a3, THREAD_HPSIR)
        lrw    r10, THREAD_SOFT  /* soft[28]*/
        add    r10, a3
        cprgr  r6, <0, 0x0>
        cprgr  r7, <0, 0x1>
        cprgr  r8, <0, 0x2>
        cprgr  r9, <0, 0x3>
        cprgr  r16, <0, 0x4>
        cprgr  r17, <0, 0x5>
        cprgr  r18, <0, 0x6>
        cprgr  r19, <0, 0x7>
        stw    r6, (r10, 0)
        stw    r7, (r10, 4)
        stw    r8, (r10, 8)
        stw    r9, (r10, 12)
        stw    r16, (r10, 16)
        stw    r17, (r10, 20)
        stw    r18, (r10, 24)
        stw    r19, (r10, 28)
        cprgr  r6, <0, 0x8>
        cprgr  r7, <0, 0x9>
        cprgr  r8, <0, 0xa>
        cprgr  r9, <0, 0xb>
        cprgr  r16, <0, 0xc>
        cprgr  r17, <0, 0xd>
        cprgr  r18, <0, 0xe>
        cprgr  r19, <0, 0xf>
        stw    r6, (r10, 32)
        stw    r7, (r10, 36)
        stw    r8, (r10, 40)
        stw    r9, (r10, 44)
        stw    r16, (r10, 48)
        stw    r17, (r10, 52)
        stw    r18, (r10, 56)
        stw    r19, (r10, 60)
        cprgr  r6, <0, 0x10>
        cprgr  r7, <0, 0x11>
        cprgr  r8, <0, 0x12>
        cprgr  r9, <0, 0x13>
        cprgr  r16, <0, 0x14>
        cprgr  r17, <0, 0x15>
        cprgr  r18, <0, 0x16>
        cprgr  r19, <0, 0x17>
        stw    r6, (r10, 64)
        stw    r7, (r10, 68)
        stw    r8, (r10, 72)
        stw    r9, (r10, 76)
        stw    r16, (r10, 80)
        stw    r17, (r10, 84)
        stw    r18, (r10, 88)
        stw    r19, (r10, 92)
        cprgr  r6, <0, 0x18>
        cprgr  r7, <0, 0x19>
        cprgr  r8, <0, 0x1a>
        cprgr  r9, <0, 0x1b>
        stw    r6, (r10, 96)
        stw    r7, (r10, 100)
        stw    r8, (r10, 104)
        stw    r9, (r10, 108)
        lrw    r10, THREAD_HARD  /* hard[30]*/
        add    r10, a3
        cprgr  r6, <0, 0x20>
        cprgr  r7, <0, 0x21>
        cprgr  r8, <0, 0x22>
        cprgr  r9, <0, 0x23>
        cprgr  r16, <0, 0x24>
        cprgr  r17, <0, 0x25>
        cprgr  r18, <0, 0x26>
        cprgr  r19, <0, 0x27>
        stw    r6, (r10, 0)
        stw    r7, (r10, 4)
        stw    r8, (r10, 8)
        stw    r9, (r10, 12)
        stw    r16, (r10, 16)
        stw    r17, (r10, 20)
        stw    r18, (r10, 24)
        stw    r19, (r10, 28)
        cprgr  r6, <0, 0x28>
        cprgr  r7, <0, 0x29>
        cprgr  r8, <0, 0x2a>
        cprgr  r9, <0, 0x2b>
        cprgr  r16, <0, 0x2c>
        cprgr  r17, <0, 0x2d>
        cprgr  r18, <0, 0x2e>
        cprgr  r19, <0, 0x2f>
        stw    r6, (r10, 32)
        stw    r7, (r10, 36)
        stw    r8, (r10, 40)
        stw    r9, (r10, 44)
        stw    r16, (r10, 48)
        stw    r17, (r10, 52)
        stw    r18, (r10, 56)
        stw    r19, (r10, 60)
        cprgr  r6, <0, 0x30>
        cprgr  r7, <0, 0x31>
        cprgr  r8, <0, 0x32>
        cprgr  r9, <0, 0x33>
        cprgr  r16, <0, 0x34>
        cprgr  r17, <0, 0x35>
        cprgr  r18, <0, 0x36>
        cprgr  r19, <0, 0x37>
        stw    r6, (r10, 64)
        stw    r7, (r10, 68)
        stw    r8, (r10, 72)
        stw    r9, (r10, 76)
        stw    r16, (r10, 80)
        stw    r17, (r10, 84)
        stw    r18, (r10, 88)
        stw    r19, (r10, 92)
        cprgr  r6, <0, 0x38>
        cprgr  r7, <0, 0x39>
        cprgr  r8, <0, 0x3a>
        cprgr  r9, <0, 0x3b>
        cprgr  r16, <0, 0x3c>
        cprgr  r17, <0, 0x3d>
        stw    r6, (r10, 96)
        stw    r7, (r10, 100)
        stw    r8, (r10, 104)
        stw    r9, (r10, 108)
        stw    r16, (r10, 112)
        stw    r17, (r10, 116)
        lrw    r10, THREAD_EXTEND  /* extend[26]*/
        add    r10, a3
        cprgr  r6, <0, 0x40>
        cprgr  r7, <0, 0x41>
        cprgr  r8, <0, 0x42>
        cprgr  r9, <0, 0x43>
        cprgr  r16, <0, 0x44>
        cprgr  r17, <0, 0x45>
        cprgr  r18, <0, 0x46>
        cprgr  r19, <0, 0x47>
        stw    r6, (r10, 0)
        stw    r7, (r10, 4)
        stw    r8, (r10, 8)
        stw    r9, (r10, 12)
        stw    r16, (r10, 16)
        stw    r17, (r10, 20)
        stw    r18, (r10, 24)
        stw    r19, (r10, 28)
        cprgr  r6, <0, 0x48>
        cprgr  r7, <0, 0x49>
        cprgr  r8, <0, 0x4a>
        cprgr  r9, <0, 0x4b>
        cprgr  r16, <0, 0x4c>
        cprgr  r17, <0, 0x4d>
        cprgr  r18, <0, 0x4e>
        cprgr  r19, <0, 0x4f>
        stw    r6, (r10, 32)
        stw    r7, (r10, 36)
        stw    r8, (r10, 40)
        stw    r9, (r10, 44)
        stw    r16, (r10, 48)
        stw    r17, (r10, 52)
        stw    r18, (r10, 56)
        stw    r19, (r10, 60)
        cprgr  r6, <0, 0x50>
        cprgr  r7, <0, 0x51>
        cprgr  r8, <0, 0x52>
        cprgr  r9, <0, 0x53>
        cprgr  r16, <0, 0x54>
        cprgr  r17, <0, 0x55>
        cprgr  r18, <0, 0x56>
        cprgr  r19, <0, 0x57>
        stw    r6, (r10, 64)
        stw    r7, (r10, 68)
        stw    r8, (r10, 72)
        stw    r9, (r10, 76)
        stw    r16, (r10, 80)
        stw    r17, (r10, 84)
        stw    r18, (r10, 88)
        stw    r19, (r10, 92)
        cprgr  r6, <0, 0x58>
        cprgr  r7, <0, 0x59>
        stw    r6, (r10, 96)
        stw    r7, (r10, 100)
.endm

.macro PRFL_RESTORE_REGS
        ldw    r6, (a3, THREAD_HPCR)
        ldw    r7, (a3, THREAD_HPSPR)
        ldw    r8, (a3, THREAD_HPEPR)
        ldw    r9, (a3, THREAD_HPSIR)
        cprcr  r6, <0, 0x0>
        cprcr  r7, <0, 0x1>
        cprcr  r8, <0, 0x2>
        cprcr  r9, <0, 0x3>
        lrw    r10, THREAD_SOFT /* soft[28] */
        add    r10, a3
        ldw    r6, (r10, 0)
        ldw    r7, (r10, 4)
        ldw    r8, (r10, 8)
        ldw    r9, (r10, 12)
        ldw    r16, (r10, 16)
        ldw    r17, (r10, 20)
        ldw    r18, (r10, 24)
        ldw    r19, (r10, 28)
        cpwgr  r6, <0, 0x0>
        cpwgr  r7, <0, 0x1>
        cpwgr  r8, <0, 0x2>
        cpwgr  r9, <0, 0x3>
        cpwgr  r16, <0, 0x4>
        cpwgr  r17, <0, 0x5>
        cpwgr  r18, <0, 0x6>
        cpwgr  r19, <0, 0x7>
        ldw    r6, (r10, 32)
        ldw    r7, (r10, 36)
        ldw    r8, (r10, 40)
        ldw    r9, (r10, 44)
        ldw    r16, (r10, 48)
        ldw    r17, (r10, 52)
        ldw    r18, (r10, 56)
        ldw    r19, (r10, 60)
        cpwgr  r6, <0, 0x8>
        cpwgr  r7, <0, 0x9>
        cpwgr  r8, <0, 0xa>
        cpwgr  r9, <0, 0xb>
        cpwgr  r16, <0, 0xc>
        cpwgr  r17, <0, 0xd>
        cpwgr  r18, <0, 0xe>
        cpwgr  r19, <0, 0xf>
        ldw    r6, (r10, 64)
        ldw    r7, (r10, 68)
        ldw    r8, (r10, 72)
        ldw    r9, (r10, 76)
        ldw    r16, (r10, 80)
        ldw    r17, (r10, 84)
        ldw    r18, (r10, 88)
        ldw    r19, (r10, 92)
        cpwgr  r6, <0, 0x10>
        cpwgr  r7, <0, 0x11>
        cpwgr  r8, <0, 0x12>
        cpwgr  r9, <0, 0x13>
        cpwgr  r16, <0, 0x14>
        cpwgr  r17, <0, 0x15>
        cpwgr  r18, <0, 0x16>
        cpwgr  r19, <0, 0x17>
        ldw    r6, (r10, 96)
        ldw    r7, (r10, 100)
        ldw    r8, (r10, 104)
        ldw    r9, (r10, 108)
        cpwgr  r6, <0, 0x18>
        cpwgr  r7, <0, 0x19>
        cpwgr  r8, <0, 0x1a>
        cpwgr  r9, <0, 0x1b>
        lrw    r10, THREAD_HARD  /*hard[30]*/
        add    r10, a3
        ldw    r6, (r10, 0)
        ldw    r7, (r10, 4)
        ldw    r8, (r10, 8)
        ldw    r9, (r10, 12)
        ldw    r16, (r10, 16)
        ldw    r17, (r10, 20)
        ldw    r18, (r10, 24)
        ldw    r19, (r10, 28)
        cpwgr  r6, <0, 0x20>
        cpwgr  r7, <0, 0x21>
        cpwgr  r8, <0, 0x22>
        cpwgr  r9, <0, 0x23>
        cpwgr  r16, <0, 0x24>
        cpwgr  r17, <0, 0x25>
        cpwgr  r18, <0, 0x26>
        cpwgr  r19, <0, 0x27>
        ldw    r6, (r10, 32)
        ldw    r7, (r10, 36)
        ldw    r8, (r10, 40)
        ldw    r9, (r10, 44)
        ldw    r16, (r10, 48)
        ldw    r17, (r10, 52)
        ldw    r18, (r10, 56)
        ldw    r19, (r10, 60)
        cpwgr  r6, <0, 0x28>
        cpwgr  r7, <0, 0x29>
        cpwgr  r8, <0, 0x2a>
        cpwgr  r9, <0, 0x2b>
        cpwgr  r16, <0, 0x2c>
        cpwgr  r17, <0, 0x2d>
        cpwgr  r18, <0, 0x2e>
        cpwgr  r19, <0, 0x2f>
        ldw    r6, (r10, 64)
        ldw    r7, (r10, 68)
        ldw    r8, (r10, 72)
        ldw    r9, (r10, 76)
        ldw    r16, (r10, 80)
        ldw    r17, (r10, 84)
        ldw    r18, (r10, 88)
        ldw    r19, (r10, 92)
        cpwgr  r6, <0, 0x30>
        cpwgr  r7, <0, 0x31>
        cpwgr  r8, <0, 0x32>
        cpwgr  r9, <0, 0x33>
        cpwgr  r16, <0, 0x34>
        cpwgr  r17, <0, 0x35>
        cpwgr  r18, <0, 0x36>
        cpwgr  r19, <0, 0x37>
        ldw    r6, (r10, 96)
        ldw    r7, (r10, 100)
        ldw    r8, (r10, 104)
        ldw    r9, (r10, 108)
        ldw    r16, (r10, 112)
        ldw    r17, (r10, 116)
        cpwgr  r6, <0, 0x38>
        cpwgr  r7, <0, 0x39>
        cpwgr  r8, <0, 0x3a>
        cpwgr  r9, <0, 0x3b>
        cpwgr  r16, <0, 0x3c>
        cpwgr  r17, <0, 0x3d>
        lrw    r10, THREAD_EXTEND  /*extend[26]*/
        add    r10, a3
        ldw    r6, (r10, 0)
        ldw    r7, (r10, 4)
        ldw    r8, (r10, 8)
        ldw    r9, (r10, 12)
        ldw    r16, (r10, 16)
        ldw    r17, (r10, 20)
        ldw    r18, (r10, 24)
        ldw    r19, (r10, 28)
        cpwgr  r6, <0, 0x40>
        cpwgr  r7, <0, 0x41>
        cpwgr  r8, <0, 0x42>
        cpwgr  r9, <0, 0x43>
        cpwgr  r16, <0, 0x44>
        cpwgr  r17, <0, 0x45>
        cpwgr  r18, <0, 0x46>
        cpwgr  r19, <0, 0x47>
        ldw    r6, (r10, 32)
        ldw    r7, (r10, 36)
        ldw    r8, (r10, 40)
        ldw    r9, (r10, 44)
        ldw    r16, (r10, 48)
        ldw    r17, (r10, 52)
        ldw    r18, (r10, 56)
        ldw    r19, (r10, 60)
        cpwgr  r6, <0, 0x48>
        cpwgr  r7, <0, 0x49>
        cpwgr  r8, <0, 0x4a>
        cpwgr  r9, <0, 0x4b>
        cpwgr  r16, <0, 0x4c>
        cpwgr  r17, <0, 0x4d>
        cpwgr  r18, <0, 0x4e>
        cpwgr  r19, <0, 0x4f>
        ldw    r6, (r10, 64)
        ldw    r7, (r10, 68)
        ldw    r8, (r10, 72)
        ldw    r9, (r10, 76)
        ldw    r16, (r10, 80)
        ldw    r17, (r10, 84)
        ldw    r18, (r10, 88)
        ldw    r19, (r10, 92)
        cpwgr  r6, <0, 0x50>
        cpwgr  r7, <0, 0x51>
        cpwgr  r8, <0, 0x52>
        cpwgr  r9, <0, 0x53>
        cpwgr  r16, <0, 0x54>
        cpwgr  r17, <0, 0x55>
        cpwgr  r18, <0, 0x56>
        cpwgr  r19, <0, 0x57>
        ldw    r6, (r10, 96)
        ldw    r7, (r10, 100)
        cpwgr  r6, <0, 0x58>
        cpwgr  r7, <0, 0x59>
.endm

#endif


#endif /* __CSKY_PRFL_H */
