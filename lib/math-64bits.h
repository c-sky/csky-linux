/*
 * math-64bits.h extracted from gcc-2.95.2/libgcc2.c which is: 
 *
 * This file is part of GNU CC.
 *
 * GNU CC is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU CC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU CC; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  
 * 
 * Copyright (C) 1989, 92-98, 1999 Free Software Foundation, Inc.
 * Copyright (C) 2009 Hangzhou C-SKY Microsystems co.,ltd. 
 *
 */

#ifndef __CSKY_MATH_64BITS_H__
#define __CSKY_MATH_64BITS_H__

#include <asm/byteorder.h>

#define BITS_PER_UNIT 8

typedef          int SItype     __attribute__ ((mode (SI)));
typedef unsigned int USItype    __attribute__ ((mode (SI)));
typedef          int DItype     __attribute__ ((mode (DI)));
typedef int word_type __attribute__ ((mode (__word__)));

#ifdef __BIG_ENDIAN
struct DIstruct {
    SItype high, low;
};
#elif defined(__LITTLE_ENDIAN)
struct DIstruct {
    SItype low, high;
};
#else
#error I feel sick.
#endif

typedef union
{
    struct DIstruct s;
    DItype ll;
} DIunion;

#endif /* __CSKY_MATH_64BITS_H__ */
