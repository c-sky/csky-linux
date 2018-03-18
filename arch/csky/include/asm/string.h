// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef _CSKY_STRING_MM_H_
#define _CSKY_STRING_MM_H_

#ifndef __ASSEMBLY__
#include <linux/types.h>
#include <linux/compiler.h>

#define __HAVE_ARCH_MEMCPY
extern void * memcpy(void *to, const void *from, size_t l);

/* New and improved.  In arch/csky/lib/memset.c */
#define __HAVE_ARCH_MEMSET
extern void * memset(void *dest, int c, size_t l);

#endif

#endif /* _CSKY_STRING_MM_H_ */
