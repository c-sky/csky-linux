/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_RISCV_SPINLOCK_H
#define __ASM_RISCV_SPINLOCK_H

#ifdef CONFIG_QUEUED_SPINLOCKS
/*
 * The KVM guests fall back to a Test-and-Set spinlock, because fair locks
 * have horrible lock 'holder' preemption issues. The virt_spin_lock_key
 * would shortcut for the queued_spin_lock_slowpath() function that allow
 * virt_spin_lock to hijack it.
 */
DECLARE_STATIC_KEY_TRUE(virt_spin_lock_key);

#define virt_spin_lock virt_spin_lock
static inline bool virt_spin_lock(struct qspinlock *lock)
{
	if (!static_branch_likely(&virt_spin_lock_key))
		return false;

	do {
		while (atomic_read(&lock->val) != 0)
			cpu_relax();
	} while (atomic_cmpxchg(&lock->val, 0, _Q_LOCKED_VAL) != 0);

	return true;
}

#define _Q_PENDING_LOOPS	(1 << 9)
#endif

#ifdef CONFIG_RISCV_COMBO_SPINLOCKS
#include <asm-generic/ticket_spinlock.h>

#undef arch_spin_is_locked
#undef arch_spin_is_contended
#undef arch_spin_value_unlocked
#undef arch_spin_lock
#undef arch_spin_trylock
#undef arch_spin_unlock

#include <asm-generic/qspinlock.h>
#include <linux/jump_label.h>

#undef arch_spin_is_locked
#undef arch_spin_is_contended
#undef arch_spin_value_unlocked
#undef arch_spin_lock
#undef arch_spin_trylock
#undef arch_spin_unlock

DECLARE_STATIC_KEY_TRUE(qspinlock_key);

static __always_inline void arch_spin_lock(arch_spinlock_t *lock)
{
	if (static_branch_likely(&qspinlock_key))
		queued_spin_lock(lock);
	else
		ticket_spin_lock(lock);
}

static __always_inline bool arch_spin_trylock(arch_spinlock_t *lock)
{
	if (static_branch_likely(&qspinlock_key))
		return queued_spin_trylock(lock);
	else
		return ticket_spin_trylock(lock);
}

static __always_inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	if (static_branch_likely(&qspinlock_key))
		queued_spin_unlock(lock);
	else
		ticket_spin_unlock(lock);
}

static __always_inline int arch_spin_value_unlocked(arch_spinlock_t lock)
{
	if (static_branch_likely(&qspinlock_key))
		return queued_spin_value_unlocked(lock);
	else
		return ticket_spin_value_unlocked(lock);
}

static __always_inline int arch_spin_is_locked(arch_spinlock_t *lock)
{
	if (static_branch_likely(&qspinlock_key))
		return queued_spin_is_locked(lock);
	else
		return ticket_spin_is_locked(lock);
}

static __always_inline int arch_spin_is_contended(arch_spinlock_t *lock)
{
	if (static_branch_likely(&qspinlock_key))
		return queued_spin_is_contended(lock);
	else
		return ticket_spin_is_contended(lock);
}
#else /* CONFIG_RISCV_COMBO_SPINLOCKS */

#ifdef CONFIG_QUEUED_SPINLOCKS
#include <asm/qspinlock.h>
#else
#include <asm-generic/ticket_spinlock.h>
#endif

#endif /* CONFIG_RISCV_COMBO_SPINLOCKS */

#include <asm/qrwlock.h>

#endif /* __ASM_RISCV_SPINLOCK_H */
