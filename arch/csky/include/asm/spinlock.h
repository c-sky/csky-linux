#ifndef __ASM_CSKY_SPINLOCK_H
#define __ASM_CSKY_SPINLOCK_H

#include <linux/spinlock_types.h>
#include <asm/barrier.h>

#define arch_spin_is_locked(x)	(READ_ONCE((x)->lock) != 0)
#ifdef CSKY_DEBUG_WITH_KERNEL_4_9
#define arch_spin_lock_flags(lock, flags)  arch_spin_lock(lock)
#define arch_read_lock_flags(lock, flags)  arch_read_lock(lock)
#define arch_write_lock_flags(lock, flags) arch_write_lock(lock)

static inline void arch_spin_unlock_wait(arch_spinlock_t *lock)
{
	smp_mb();
	while(arch_spin_is_locked(lock));
	smp_mb();
}
#endif

/****** spin lock/unlock/trylock ******/
static inline void arch_spin_lock(arch_spinlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	bnez		%0, 1b   \n"
		"	movi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();
}

static inline void arch_spin_unlock(arch_spinlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	movi		%0, 0    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();
}

static inline int arch_spin_trylock(arch_spinlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	bnez		%0, 2f   \n"
		"	movi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		"	movi		%0, 0    \n"
		"2:				 \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();

	return !tmp;
}

/****** read lock/unlock/trylock ******/
static inline void arch_read_lock(arch_rwlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	blz		%0, 1b   \n"
		"	addi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();
}

static inline void arch_read_unlock(arch_rwlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	subi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();
}

static inline int arch_read_trylock(arch_rwlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	blz		%0, 2f   \n"
		"	addi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		"	movi		%0, 0    \n"
		"2:				 \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();

	return !tmp;
}

/****** write lock/unlock/trylock ******/
static inline void arch_write_lock(arch_rwlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	bnez		%0, 1b   \n"
		"	subi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();
}

static inline void arch_write_unlock(arch_rwlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	movi		%0, 0    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();
}

static inline int arch_write_trylock(arch_rwlock_t *lock)
{
	unsigned int *p = &lock->lock;
	unsigned int tmp;

	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%1) \n"
		"	bnez		%0, 2f   \n"
		"	subi		%0, 1    \n"
		"	stex.w		%0, (%1) \n"
		"	bez		%0, 1b   \n"
		"	movi		%0, 0    \n"
		"2:				 \n"
		: "=&r" (tmp)
		: "r"(p)
		: "memory");
	smp_mb();

	return !tmp;
}

#endif /* __ASM_CSKY_SPINLOCK_H */
