/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  (C) Copyright 2015, Chen Linfei (linfei_chen@c-sky.com)
 *
 */

#ifndef __CSKY_READREG_H
#define __CSKY_READREG_H

#ifdef __CSKYABIV1__
static inline unsigned long csky_get_sp(void)
{
        int __res;
        __asm__ __volatile__("mov %0, r0\n\t"
                             :"=b" (__res));
        return   __res;
}

static inline unsigned long read_cpu_ss1(void)
{
        int __res;
        __asm__ __volatile__("mfcr %0, ss1\n\t"
                             :"=b" (__res));
        return   __res;
}
#else
static inline unsigned long csky_get_sp(void)
{
        int __res;
        __asm__ __volatile__("mov %0, sp\n\t"
                             :"=r" (__res));
        return   __res;
}

static inline unsigned long read_cpu_usp(void)
{
        int __res;
        __asm__ __volatile__("mfcr %0, cr<14,1>\n\t"
                                        :"=r" (__res));
        return   __res;
}
#endif

#endif
