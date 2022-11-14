/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_CSKY_DELAY_H
#define _ASM_CSKY_DELAY_H

extern unsigned long csky_timebase;

#define udelay udelay
extern void udelay(unsigned long usecs);

#define ndelay ndelay
extern void ndelay(unsigned long nsecs);

extern void __delay(unsigned long cycles);

#endif /* _ASM_CSKY_DELAY_H */
