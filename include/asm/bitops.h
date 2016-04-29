#ifndef __ASM_CSKY_BITOPS_H
#define __ASM_CSKY_BITOPS_H

#include <linux/irqflags.h>
#include <linux/compiler.h>
#include <asm/barrier.h>

/*
 * asm-generic/bitops/ffs.h
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

/*
 * asm-generic/bitops/__ffs.h
 */
static inline int __ffs(unsigned int x)
{
	__asm__ __volatile__ (
			"brev %0\n\t"
			"ff1  %0\n\t"
			: "=r"(x)
			: "0"(x));
	return x;
}

#include <asm-generic/bitops/ffz.h>

/* 
 * asm-generic/bitops/find.h
 */
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

/*
 * asm-generic/bitops/fls.h
 */
static inline int fls(int x)
{
	__asm__ __volatile__(
			"ff1 %0\n\t"
			:"=r" (x)
			:"0" (x));

	return (32 - x);
}

/*
 * asm-generic/bitops/__fls.h
 */
static inline int __fls(int x)
{
	return fls(x) - 1;
}

#include <asm-generic/bitops/fls64.h>
#include <asm-generic/bitops/find.h>

#ifndef _LINUX_BITOPS_H
#error only <linux/bitops.h> can be included directly
#endif

#include <asm-generic/bitops/sched.h>
#include <asm-generic/bitops/hweight.h>
#include <asm-generic/bitops/lock.h>

#include <asm-generic/bitops/atomic.h>

/*
 * bug fix, why only could use atomic!!!!
 */
#include <asm-generic/bitops/non-atomic.h>
#define __clear_bit(nr,vaddr) clear_bit(nr,vaddr)

#include <asm-generic/bitops/le.h>
#include <asm-generic/bitops/ext2-atomic.h>
#endif /* __ASM_CSKY_BITOPS_H */

