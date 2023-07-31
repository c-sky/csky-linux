/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RISCV_QSPINLOCK_PARAVIRT_H
#define _ASM_RISCV_QSPINLOCK_PARAVIRT_H

void __pv_queued_spin_unlock(struct qspinlock *lock);

#endif /* _ASM_RISCV_QSPINLOCK_PARAVIRT_H */
