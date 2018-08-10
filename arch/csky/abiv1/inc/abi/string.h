// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef _ABI_STRING_H_
#define _ABI_STRING_H_

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *, const void *, __kernel_size_t);
extern void *__memcpy(void *, const void *, __kernel_size_t);

#define __HAVE_ARCH_MEMSET
extern void *memset(void *, int, __kernel_size_t);
extern void *__memset(void *, int, __kernel_size_t);

#endif /* _ABI_STRING_H_ */
