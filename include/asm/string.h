/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  (C) Copyright 2004, Li Chunqiang (chunqiang_li@c-sky.com)
 *  (C) Copyright 2009, Hu Junshan (junshan_hu@c-sky.com)
 *  (C) Copyright 2009, C-SKY Microsystems Co., Ltd. (www.c-sky.com)
 *  
 */ 
  
#ifndef _CSKY_STRING_MM_H_
#define _CSKY_STRING_MM_H_

#ifndef __ASSEMBLY___
#include <linux/types.h>
#include <linux/compiler.h>

#define __HAVE_ARCH_MEMCPY
extern void * memcpy(void *to, const void *from, size_t l);

/* New and improved.  In arch/csky/lib/memset.c */
#define __HAVE_ARCH_MEMSET
extern void * memset(void *dest, int c, size_t l);

#endif

#endif /* _CSKY_STRING_MM_H_ */
