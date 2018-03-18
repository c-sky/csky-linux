// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_BARRIER_H
#define __ASM_CSKY_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()	asm volatile ("nop")
#define mb()	asm volatile ("sync":::"memory")

#include <asm-generic/barrier.h>

#endif /* __ASSEMBLY__ */
#endif /* __ASM_CSKY_BARRIER_H */
