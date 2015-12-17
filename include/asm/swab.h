/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  
 */ 
  
#ifndef _CSKY_SWAB_H
#define _CSKY_SWAB_H

#include <linux/types.h>
#include <linux/compiler.h>

#ifdef CONFIG_CPU_CSKYV2
static inline __attribute_const__ __u16 __arch_swab16(__u16 x)
{
	__asm__ ("revh %0, %1" : "=r" (x) : "r" (x));
	return x;
}
#define __arch_swab16 __arch_swab16

static inline __attribute_const__ __u32 __arch_swab32(__u32 x)
{
	__asm__ ("revb %0, %1" : "=r" (x) : "r" (x));
	return x;
}
#define __arch_swab32 __arch_swab32
#endif

#endif /* _CSKY_SWAB_H */
