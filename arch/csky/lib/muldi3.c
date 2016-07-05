/*
 * muldi3.c extracted from gcc-2.95.2/libgcc2.c which is: 
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

#include "math-64bits.h"

#ifndef W_TYPE_SIZE
#define W_TYPE_SIZE 32
#define UWtype      USItype
#define UHWtype     USItype
#define UDWtype     UDItype
#endif

#define __ll_B ((UWtype) 1 << (W_TYPE_SIZE / 2))
#define __ll_lowpart(t) ((UWtype) (t) & (__ll_B - 1))
#define __ll_highpart(t) ((UWtype) (t) >> (W_TYPE_SIZE / 2))


#define umul_ppmm(w1, w0, u, v)                     \
  do {                                              \
    UWtype __x0, __x1, __x2, __x3;                  \
    UHWtype __ul, __vl, __uh, __vh;                 \
                                                    \
    __ul = __ll_lowpart (u);                        \
    __uh = __ll_highpart (u);                       \
    __vl = __ll_lowpart (v);                        \
    __vh = __ll_highpart (v);                       \
                                                    \
    __x0 = (UWtype) __ul * __vl;                    \
    __x1 = (UWtype) __ul * __vh;                    \
    __x2 = (UWtype) __uh * __vl;                    \
    __x3 = (UWtype) __uh * __vh;                    \
                                                    \
    __x1 += __ll_highpart (__x0);/* this can't give carry */             \
    __x1 += __x2;                /* but this indeed can */               \
    if (__x1 < __x2)             /* did we get it? */                    \
      __x3 += __ll_B;            /* yes, add it in the proper pos.  */   \
                                                                         \
    (w1) = __x3 + __ll_highpart (__x1);                                  \
    (w0) = __ll_lowpart (__x1) * __ll_B + __ll_lowpart (__x0);           \
  } while (0)

#define __umulsidi3(u, v) \
  ({DIunion __w;                                                        \
    umul_ppmm (__w.s.high, __w.s.low, u, v);                            \
    __w.ll; })

DItype
__muldi3 (DItype u, DItype v)
{
	DIunion w;
	DIunion uu, vv;

	uu.ll = u,
  	vv.ll = v;

	w.ll = __umulsidi3 (uu.s.low, vv.s.low);
  	w.s.high += ((USItype) uu.s.low * (USItype) vv.s.high
               + (USItype) uu.s.high * (USItype) vv.s.low);
	return w.ll;
}
