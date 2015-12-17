/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright 1992, Linus Torvalds.
 * Copyright (C) 2009 Hangzhou C-SKY Microsystems co.,ltd. 
 */

#ifndef _CSKY_BITOPS_H
#define _CSKY_BITOPS_H

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <linux/compiler.h>
#include <linux/types.h>
//#include <asm/bug.h>
#include <asm/byteorder.h>
#include <asm/irqflags.h>
#include <asm/barrier.h>

extern int printk(const char *fmt, ...);
static inline int test_and_set_bit(int nr, volatile void * addr)
{
	int     mask, retval;
	volatile unsigned int *a = (volatile unsigned int *) addr;
	unsigned long flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	flags = arch_local_irq_save();
	retval = (mask & *a) != 0;
	*a |= mask;
	arch_local_irq_restore(flags);

	return retval;
}

static inline int __test_and_set_bit(int nr, volatile void * addr)
{
	int     mask, retval;
	volatile unsigned int *a = (volatile unsigned int *) addr;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	retval = (mask & *a) != 0;
	*a |= mask;
	return retval;
}

static inline void set_bit(int nr, volatile void * addr)
{
	volatile unsigned int *a = (volatile unsigned int *) addr;
	int     mask;
	unsigned long flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	flags = arch_local_irq_save();
	*a |= mask;
	arch_local_irq_restore(flags);
}
static inline  void __set_bit(int nr, volatile void * addr)
{
	volatile unsigned int *a = (volatile unsigned int *) addr;
	int     mask;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	*a |= mask;
}

static inline int test_and_clear_bit(int nr, volatile void * addr)
{
	int     mask, retval;
	volatile unsigned int *a = (volatile unsigned int *) addr;
	unsigned long flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	flags = arch_local_irq_save();
	retval = (mask & *a) != 0;
	*a &= ~mask;
	arch_local_irq_restore(flags);

	return retval;
}

static inline int __test_and_clear_bit(int nr, volatile void * addr)
{
	int     mask, retval;
	volatile unsigned int *a = (volatile unsigned int *) addr;

	a += nr >> 5; 
	mask = 1 << (nr & 0x1f);
	retval = (mask & *a) != 0;
	*a &= ~mask;
	return retval;
}

/*
 * clear_bit() doesn't provide any barrier for the compiler.
 */
#define smp_mb__before_clear_bit()      barrier()
#define smp_mb__after_clear_bit()       barrier()


static inline void clear_bit(int nr, volatile void * addr)
{       
	volatile unsigned int *a = (volatile unsigned int *) addr;
	int     mask;
	unsigned long flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	flags = arch_local_irq_save();
	*a &= ~mask;
	arch_local_irq_restore(flags);
}

#define __clear_bit(nr,vaddr) clear_bit(nr,vaddr)

static inline int test_and_change_bit(int nr, volatile void * addr)
{
	int     mask, retval;
	volatile unsigned int *a = (volatile unsigned int *) addr;
	unsigned long flags;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	flags = arch_local_irq_save();
	retval = (mask & *a) != 0;
	*a ^= mask;
	arch_local_irq_restore(flags);

	return retval;
}

static inline int __test_and_change_bit(int nr, volatile void * addr)
{
	int     mask, retval;
	volatile unsigned int *a = (volatile unsigned int *) addr;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	retval = (mask & *a) != 0;
	*a ^= mask;
	return retval;
}

static inline void change_bit(int nr, volatile void * addr)
{
	int mask;
	unsigned long flags;
	volatile unsigned long *ADDR = (volatile unsigned long *) addr;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	flags = arch_local_irq_save();
	*ADDR ^= mask;
	arch_local_irq_restore(flags);
}

static inline void __change_bit(int nr, volatile void * addr)
{
	int mask;
	volatile unsigned long *ADDR = (volatile unsigned long *) addr;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	*ADDR ^= mask;
}

static inline int __test_bit(int nr, volatile void * addr)
{
	volatile unsigned int *a = (volatile unsigned int *) addr;
	int     mask;

	a += nr >> 5;
	mask = 1 << (nr & 0x1f);
	return ((mask & *a) != 0);
}

static inline int test_bit(int nr, const volatile unsigned long *vaddr)
{
	return (vaddr[nr >> 5] & (1UL << (nr & 31))) != 0;
}

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int ffs(int x)
{
	if(!x)
	{
		return 0;
	}
	__asm__ __volatile__ (
			"brev %0\n\t"
			"ff1  %0\n\t"
			"addi %0, 1\n\t"  
			: "=r"(x)  
			: "0"(x));
	return x;
}

static inline int __ffs(unsigned int x)
{
	__asm__ __volatile__ (
			"brev %0\n\t"
			"ff1  %0\n\t"
			: "=r"(x)
			: "0"(x));
	return x;	
}

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
static inline  unsigned long ffz(unsigned long word)
{
	unsigned long result = ~word;
	__asm__ __volatile__ (
			"brev %0\n\t"
			"ff1  %0\n\t"
			: "=r"(result)
			: "0"(result));
	return result; 
}

static inline int find_next_zero_bit (const unsigned long * addr, 
		int size, int offset)
{
	const unsigned long *p = addr + (offset >> 5);
	int result = offset & ~31UL;
	int tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if (offset) {
		tmp = *(p++);
		tmp |= ~0UL >> (32-offset);
		if (size < 32)
			goto found_first;
		if (~tmp)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while (size & ~31UL) {
		if (~(tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp |= ~0UL << size;
	if (tmp == ~0UL)        /* Are any bits zero? */
		return result + size;   /* Nope. */

found_middle:
	return result + ffz(tmp);
}
#define find_next_zero_bit find_next_zero_bit

/*
 * Find the first cleared bit in a memory region.
 */
static inline int find_first_zero_bit(const unsigned long *addr, int size)
{
	const unsigned long *p = addr;
	int result = 0;
	int tmp;

	while (size & ~(32 - 1)) {
		if (~(tmp = *(p++)))
			goto found;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;

	tmp = (*p) | (~0UL << size);
	if (tmp == ~0UL)        /* Are any bits zero? */
		return result + size;   /* Nope. */
found:
	return result + ffz(tmp);
}
#define find_first_zero_bit find_first_zero_bit

/*
 * find_next_bit - find the next set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */
static inline int find_next_bit (const unsigned long * addr, 
		int size, int offset)
{
	const unsigned long *p = addr + (offset >> 5);
	int result = offset & ~31UL;
	int tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if (offset) {
		tmp = *(p++);
		tmp &= (~0UL << offset);
		if (size < 32)
			goto found_first;
		if (tmp)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while (size & ~31UL) {
		if ((tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	tmp &= (~0UL >> (32 - size));
	if (tmp == 0UL)         /* Are any bits set? */
		return result + size;   /* Nope. */	
found_middle:
	return result + __ffs(tmp);
}

#define find_next_bit find_next_bit

/*
 * find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit-number of the first set bit, not the number of the byte
 * containing a bit.
 */
static inline int find_first_bit (const unsigned long * addr, int size)
{
	const unsigned long *p = addr;
	int result = 0;
	int tmp;

	while (size & ~31UL) {
		if ((tmp = *(p++)))
			goto found;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;

	tmp = (*p) & (~0UL >> (32 - size));
	if (tmp == 0UL)         /* Are any bits set? */
		return result + size;   /* Nope. */
found:
	return result + __ffs(tmp);
}
#define find_first_bit find_first_bit

/*
 * fls: find last bit set.
 */
static inline int fls(int x)
{
	__asm__ __volatile__(
			"ff1 %0\n\t"
			:"=r" (x)
			:"0" (x));

	return (32 - x); 
}

static inline int __fls(int x)
{
	return fls(x) - 1;
}

#include <asm-generic/bitops/fls64.h>


#ifdef __KERNEL__

#include <asm-generic/bitops/sched.h>
#include <asm-generic/bitops/hweight.h>
#include <asm-generic/bitops/lock.h>
#include <asm-generic/bitops/le.h>
#include <asm-generic/bitops/ext2-atomic.h>

#endif /* __KERNEL__ */


#endif
