/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RISCV_QSPINLOCK_H
#define _ASM_RISCV_QSPINLOCK_H

#ifdef CONFIG_PARAVIRT_SPINLOCKS
#include <asm/paravirt.h>

/* How long a lock should spin before we consider blocking */
#define SPIN_THRESHOLD		(1 << 15)

void native_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);
void __pv_init_lock_hash(void);
void __pv_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);

#ifdef CONFIG_NUMA_AWARE_SPINLOCKS
bool cna_configure_spin_lock_slowpath(void);
void __cna_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);
#endif

static inline void queued_spin_lock_slowpath(struct qspinlock *lock, u32 val)
{
	static_call(pv_queued_spin_lock_slowpath)(lock, val);
}

#define queued_spin_unlock	queued_spin_unlock
static inline void queued_spin_unlock(struct qspinlock *lock)
{
	static_call(pv_queued_spin_unlock)(lock);
}
#endif /* CONFIG_PARAVIRT_SPINLOCKS */

#include <asm-generic/qspinlock.h>

#endif /* _ASM_RISCV_QSPINLOCK_H */
