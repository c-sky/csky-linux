// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ASM_CSKY_BARRIER_H
#define __ASM_CSKY_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()	asm volatile ("nop")

/*
 * sync:        completion barrier
 * sync.s:      completion barrier and shareable to other cores
 * sync.i:      completion barrier with flush cpu pipeline
 * sync.is:     completion barrier with flush cpu pipeline and shareable to other cores
 *
 * bar.brwarw:  ordering barrier for all load/store instructions before it
 * bar.brwarws: ordering barrier for all load/store instructions before it and shareable to other cores
 * bar.brar:    ordering barrier for all load       instructions before it
 * bar.brars:   ordering barrier for all load       instructions before it and shareable to other cores
 * bar.bwaw:    ordering barrier for all store      instructions before it
 * bar.bwaws:   ordering barrier for all store      instructions before it and shareable to other cores
 */

#ifdef CONFIG_CPU_HAS_CACHEV2
#define mb()		asm volatile ("bar.brwarw":::"memory")
#define rmb()		asm volatile ("bar.brar":::"memory")
#define wmb()		asm volatile ("bar.bwaw":::"memory")

#ifdef CONFIG_SMP
#define __smp_mb()	asm volatile ("bar.brwarws":::"memory")
#define __smp_rmb()	asm volatile ("bar.brars":::"memory")
#define __smp_wmb()	asm volatile ("bar.bwaws":::"memory")
#endif /* CONFIG_SMP */

#define sync_is()	asm volatile ("sync.is":::"memory")

#else /* !CONFIG_CPU_HAS_CACHEV2 */
#define mb()		asm volatile ("sync":::"memory")
#endif

#include <asm-generic/barrier.h>

#endif /* __ASSEMBLY__ */
#endif /* __ASM_CSKY_BARRIER_H */
