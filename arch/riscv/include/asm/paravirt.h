/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RISCV_PARAVIRT_H
#define _ASM_RISCV_PARAVIRT_H

#ifdef CONFIG_PARAVIRT
#include <linux/static_call_types.h>

struct static_key;
extern struct static_key paravirt_steal_enabled;
extern struct static_key paravirt_steal_rq_enabled;

u64 dummy_steal_clock(int cpu);

DECLARE_STATIC_CALL(pv_steal_clock, dummy_steal_clock);

static inline u64 paravirt_steal_clock(int cpu)
{
	return static_call(pv_steal_clock)(cpu);
}

int __init pv_time_init(void);

#else

#define pv_time_init() do {} while (0)

#endif // CONFIG_PARAVIRT

#ifdef CONFIG_PARAVIRT_SPINLOCKS

void pv_wait(u8 *ptr, u8 val);
void pv_kick(int cpu);

void dummy_queued_spin_lock_slowpath(struct qspinlock *lock, u32 val);
void dummy_queued_spin_unlock(struct qspinlock *lock);

DECLARE_STATIC_CALL(pv_queued_spin_lock_slowpath, dummy_queued_spin_lock_slowpath);
DECLARE_STATIC_CALL(pv_queued_spin_unlock, dummy_queued_spin_unlock);

void __init pv_qspinlock_init(void);

static inline bool pv_is_native_spin_unlock(void)
{
	return false;
}

#endif /* CONFIG_PARAVIRT_SPINLOCKS */

#endif
