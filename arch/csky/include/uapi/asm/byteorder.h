// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_BYTEORDER_H
#define __ASM_CSKY_BYTEORDER_H

#if defined(__cskyBE__)
#include <linux/byteorder/big_endian.h>
#elif defined(__cskyLE__)
#include <linux/byteorder/little_endian.h>
#else
# error "There is no __cskyBE__, __cskyLE__"
#endif

#endif /* __ASM_CSKY_BYTEORDER_H */
