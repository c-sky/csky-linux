// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ASM_CSKY_BITOPS_H
#define __ASM_CSKY_BITOPS_H

#include <linux/compiler.h>
#include <asm/barrier.h>

/*
 * asm-generic/bitops/ffs.h
 */
static inline int ffs(int x)
{
	if(!x) return 0;

	asm volatile (
		"brev %0\n"
		"ff1  %0\n"
		"addi %0, 1\n"
		:"=&r"(x)
		:"0"(x));
	return x;
}

/*
 * asm-generic/bitops/__ffs.h
 */
static __always_inline unsigned long __ffs(unsigned long x)
{
	asm volatile (
		"brev %0\n"
		"ff1  %0\n"
		:"=&r"(x)
		:"0"(x));
	return x;
}

/*
 * asm-generic/bitops/fls.h
 */
static __always_inline int fls(int x)
{
	asm volatile(
		"ff1 %0\n"
		:"=&r"(x)
		:"0"(x));

	return (32 - x);
}

/*
 * asm-generic/bitops/__fls.h
 */
static __always_inline unsigned long __fls(unsigned long x)
{
	return fls(x) - 1;
}

#include <asm-generic/bitops/ffz.h>
#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/find.h>

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <asm-generic/bitops/sched.h>
#include <asm-generic/bitops/hweight.h>
#include <asm-generic/bitops/lock.h>

#ifdef CONFIG_CPU_HAS_LDSTEX

/*
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 *
 * Note: there are no guarantees that this function will not be reordered
 * on non x86 architectures, so if you are writing portable code,
 * make sure not to rely on its reordering guarantees.
 *
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline void set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long tmp;

	/* *p  |= mask; */
	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%2)	\n"
		"	or32		%0, %0, %1	\n"
		"	stex.w		%0, (%2)	\n"
		"	bez		%0, 1b		\n"
		: "=&r"(tmp)
		: "r"(mask), "r"(p)
		: "memory");
	smp_mb();
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_atomic() and/or smp_mb__after_atomic()
 * in order to ensure changes are visible on other processors.
 */
static inline void clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long tmp;

	/* *p &= ~mask; */
	mask = ~mask;
	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%2)	\n"
		"	and32		%0, %0, %1	\n"
		"	stex.w		%0, (%2)	\n"
		"	bez		%0, 1b		\n"
		: "=&r"(tmp)
		: "r"(mask), "r"(p)
		: "memory");
	smp_mb();
}

/**
 * change_bit - Toggle a bit in memory
 * @nr: Bit to change
 * @addr: Address to start counting from
 *
 * change_bit() is atomic and may not be reordered. It may be
 * reordered on other architectures than x86.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static inline void change_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long tmp;

	/* *p ^= mask; */
	smp_mb();
	asm volatile (
		"1:	ldex.w		%0, (%2)	\n"
		"	xor32		%0, %0, %1	\n"
		"	stex.w		%0, (%2)	\n"
		"	bez		%0, 1b		\n"
		: "=&r"(tmp)
		: "r"(mask), "r"(p)
		: "memory");
	smp_mb();
}

/**
 * test_and_set_bit - Set a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It may be reordered on other architectures than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old, tmp;

	/*
	 * old = *p;
	 * *p = old | mask;
	 */
	smp_mb();
	asm volatile (
		"1:	ldex.w		%1, (%3)	\n"
		"	mov		%0, %1		\n"
		"	or32		%0, %0, %2	\n"
		"	stex.w		%0, (%3)	\n"
		"	bez		%0, 1b		\n"
		: "=&r"(tmp), "=&r"(old)
		: "r"(mask), "r"(p)
		: "memory");
	smp_mb();

	return (old & mask) != 0;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It can be reorderdered on other architectures other than x86.
 * It also implies a memory barrier.
 */
static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old, tmp, mask_not;

	/*
	 * old = *p;
	 * *p = old & ~mask;
	 */
	smp_mb();
	mask_not = ~mask;
	asm volatile (
		"1:	ldex.w		%1, (%3)	\n"
		"	mov		%0, %1		\n"
		"	and32		%0, %0, %2	\n"
		"	stex.w		%0, (%3)	\n"
		"	bez		%0, 1b		\n"
		: "=&r"(tmp), "=&r"(old)
		: "r"(mask_not), "r"(p)
		: "memory");

	smp_mb();

	return (old & mask) != 0;
}

/**
 * test_and_change_bit - Change a bit and return its old value
 * @nr: Bit to change
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_change_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old, tmp;

	/*
	 * old = *p;
	 * *p = old ^ mask;
	 */
	smp_mb();
	asm volatile (
		"1:	ldex.w		%1, (%3)	\n"
		"	mov		%0, %1		\n"
		"	xor32		%0, %0, %2	\n"
		"	stex.w		%0, (%3)	\n"
		"	bez		%0, 1b		\n"
		: "=&r"(tmp), "=&r"(old)
		: "r"(mask), "r"(p)
		: "memory");
	smp_mb();

	return (old & mask) != 0;
}

#else
#include <asm-generic/bitops/atomic.h>
#endif

/*
 * bug fix, why only could use atomic!!!!
 */
#include <asm-generic/bitops/non-atomic.h>
#define __clear_bit(nr,vaddr) clear_bit(nr,vaddr)

#include <asm-generic/bitops/le.h>
#include <asm-generic/bitops/ext2-atomic.h>
#endif /* __ASM_CSKY_BITOPS_H */
