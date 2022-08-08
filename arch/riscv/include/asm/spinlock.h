/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_RISCV_SPINLOCK_H
#define __ASM_RISCV_SPINLOCK_H

#ifdef CONFIG_QUEUED_SPINLOCKS
#define _Q_PENDING_LOOPS	(1 << 9)
#endif

#ifdef CONFIG_QUEUED_SPINLOCKS
#include <asm/qspinlock.h>
#include <asm/qrwlock.h>
#else
#include <asm-generic/spinlock.h>
#endif

#endif /* __ASM_RISCV_SPINLOCK_H */
