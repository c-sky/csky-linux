/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2014 Regents of the University of California
 */

#ifndef _ASM_RISCV_CMPXCHG_H
#define _ASM_RISCV_CMPXCHG_H

#include <linux/bug.h>

#include <asm/barrier.h>
#include <asm/fence.h>

static inline ulong __xchg16_relaxed(ulong new, void *ptr)
{
	ulong ret, tmp;
	ulong shif = ((ulong)ptr & 2) ? 16 : 0;
	ulong mask = 0xffff << shif;
	ulong *__ptr = (ulong *)((ulong)ptr & ~2);

	__asm__ __volatile__ (
		"0:	lr.w %0, %2\n"
		"	and  %1, %0, %z3\n"
		"	or   %1, %1, %z4\n"
		"	sc.w %1, %1, %2\n"
		"	bnez %1, 0b\n"
		: "=&r" (ret), "=&r" (tmp), "+A" (*__ptr)
		: "rJ" (~mask), "rJ" (new << shif)
		: "memory");

	return (ulong)((ret & mask) >> shif);
}

#define __xchg_relaxed(ptr, new, size)					\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(new) __new = (new);					\
	__typeof__(*(ptr)) __ret;					\
	switch (size) {							\
	case 2:								\
		__ret = (__typeof__(*(ptr)))				\
			__xchg16_relaxed((ulong)__new, __ptr);		\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"	amoswap.w %0, %2, %1\n"			\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"	amoswap.d %0, %2, %1\n"			\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_xchg_relaxed(ptr, x)					\
({									\
	__typeof__(*(ptr)) _x_ = (x);					\
	(__typeof__(*(ptr))) __xchg_relaxed((ptr),			\
					    _x_, sizeof(*(ptr)));	\
})

#define __xchg_acquire(ptr, new, size)					\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(new) __new = (new);					\
	__typeof__(*(ptr)) __ret;					\
	switch (size) {							\
	case 4:								\
		__asm__ __volatile__ (					\
			"	amoswap.w %0, %2, %1\n"			\
			RISCV_ACQUIRE_BARRIER				\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"	amoswap.d %0, %2, %1\n"			\
			RISCV_ACQUIRE_BARRIER				\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_xchg_acquire(ptr, x)					\
({									\
	__typeof__(*(ptr)) _x_ = (x);					\
	(__typeof__(*(ptr))) __xchg_acquire((ptr),			\
					    _x_, sizeof(*(ptr)));	\
})

static inline ulong __xchg8(ulong new, void *ptr)
{
	ulong ret, tmp;
	ulong shif = ((ulong)ptr & 3) * 8;
	ulong mask = 0xff << shif;
	ulong *__ptr = (ulong *)((ulong)ptr & ~3);

	__asm__ __volatile__ (
		"0:	lr.w %0, %2\n"
		"	and  %1, %0, %z3\n"
		"	or   %1, %1, %z4\n"
		"	sc.w.rl %1, %1, %2\n"
		"	bnez %1, 0b\n"
			"fence w, rw\n"
		: "=&r" (ret), "=&r" (tmp), "+A" (*__ptr)
		: "rJ" (~mask), "rJ" (new << shif)
		: "memory");

	return (ulong)((ret & mask) >> shif);
}

#define __arch_xchg(ptr, new, size)					\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(new) __new = (new);					\
	__typeof__(*(ptr)) __ret;					\
	switch (size) {							\
	case 1:								\
		__ret = (__typeof__(*(ptr)))				\
			__xchg8((ulong)__new, __ptr);			\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"	amoswap.w.aqrl %0, %2, %1\n"		\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"	amoswap.d.aqrl %0, %2, %1\n"		\
			: "=r" (__ret), "+A" (*__ptr)			\
			: "r" (__new)					\
			: "memory");					\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_xchg(ptr, x)						\
({									\
	__typeof__(*(ptr)) _x_ = (x);					\
	(__typeof__(*(ptr))) __arch_xchg((ptr), _x_, sizeof(*(ptr)));	\
})

/*
 * Atomic compare and exchange.  Compare OLD with MEM, if identical,
 * store NEW in MEM.  Return the initial value in MEM.  Success is
 * indicated by comparing RETURN with OLD.
 */
static inline ulong __cmpxchg_small_relaxed(void *ptr, ulong old,
					    ulong new, ulong size)
{
	ulong shift;
	ulong ret, mask, temp;
	volatile ulong *ptr32;

	/* Mask inputs to the correct size. */
	mask = GENMASK((size * BITS_PER_BYTE) - 1, 0);
	old &= mask;
	new &= mask;

	/*
	 * Calculate a shift & mask that correspond to the value we wish to
	 * compare & exchange within the naturally aligned 4 byte integer
	 * that includes it.
	 */
	shift = (ulong)ptr & 0x3;
	shift *= BITS_PER_BYTE;
	old <<= shift;
	new <<= shift;
	mask <<= shift;

	/*
	 * Calculate a pointer to the naturally aligned 4 byte integer that
	 * includes our byte of interest, and load its value.
	 */
	ptr32 = (volatile ulong *)((ulong)ptr & ~0x3);

	__asm__ __volatile__ (
		"0:	lr.w %0, %2\n"
		"	and  %1, %0, %z3\n"
		"	bne  %1, %z5, 1f\n"
		"	and  %1, %0, %z4\n"
		"	or   %1, %1, %z6\n"
		"	sc.w %1, %1, %2\n"
		"	bnez %1, 0b\n"
		"1:\n"
		: "=&r" (ret), "=&r" (temp), "+A" (*ptr32)
		: "rJ" (mask), "rJ" (~mask), "rJ" (old), "rJ" (new)
		: "memory");

	return (ret & mask) >> shift;
}

#define __cmpxchg_relaxed(ptr, old, new, size)				\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(ptr)) __old = (old);				\
	__typeof__(*(ptr)) __new = (new);				\
	__typeof__(*(ptr)) __ret;					\
	register unsigned int __rc;					\
	switch (size) {							\
	case 1:								\
		__ret = (__typeof__(*(ptr)))				\
			__cmpxchg_small_relaxed(__ptr, (ulong)__old,	\
					(ulong)__new, (ulong)size);	\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"0:	lr.w %0, %2\n"				\
			"	bne  %0, %z3, 1f\n"			\
			"	sc.w %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" ((long)__old), "rJ" (__new)		\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"0:	lr.d %0, %2\n"				\
			"	bne %0, %z3, 1f\n"			\
			"	sc.d %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_cmpxchg_relaxed(ptr, o, n)					\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg_relaxed((ptr),			\
					_o_, _n_, sizeof(*(ptr)));	\
})

static inline ulong __cmpxchg_small_acquire(void *ptr, ulong old,
					    ulong new, ulong size)
{
	ulong shift;
	ulong ret, mask, temp;
	volatile ulong *ptr32;

	/* Mask inputs to the correct size. */
	mask = GENMASK((size * BITS_PER_BYTE) - 1, 0);
	old &= mask;
	new &= mask;

	/*
	 * Calculate a shift & mask that correspond to the value we wish to
	 * compare & exchange within the naturally aligned 4 byte integer
	 * that includes it.
	 */
	shift = (ulong)ptr & 0x3;
	shift *= BITS_PER_BYTE;
	old <<= shift;
	new <<= shift;
	mask <<= shift;

	/*
	 * Calculate a pointer to the naturally aligned 4 byte integer that
	 * includes our byte of interest, and load its value.
	 */
	ptr32 = (volatile ulong *)((ulong)ptr & ~0x3);

	__asm__ __volatile__ (
		"0:	lr.w %0, %2\n"
		"	and  %1, %0, %z3\n"
		"	bne  %1, %z5, 1f\n"
		"	and  %1, %0, %z4\n"
		"	or   %1, %1, %z6\n"
		"	sc.w %1, %1, %2\n"
		"	bnez %1, 0b\n"
		RISCV_ACQUIRE_BARRIER
		"1:\n"
		: "=&r" (ret), "=&r" (temp), "+A" (*ptr32)
		: "rJ" (mask), "rJ" (~mask), "rJ" (old), "rJ" (new)
		: "memory");

	return (ret & mask) >> shift;
}

#define __cmpxchg_acquire(ptr, old, new, size)				\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(ptr)) __old = (old);				\
	__typeof__(*(ptr)) __new = (new);				\
	__typeof__(*(ptr)) __ret;					\
	register unsigned int __rc;					\
	switch (size) {							\
	case 1:								\
	case 2:								\
		__ret = (__typeof__(*(ptr)))				\
			__cmpxchg_small_acquire(__ptr, (ulong)__old,	\
					(ulong)__new, (ulong)size);	\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"0:	lr.w %0, %2\n"				\
			"	bne  %0, %z3, 1f\n"			\
			"	sc.w %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			RISCV_ACQUIRE_BARRIER				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" ((long)__old), "rJ" (__new)		\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"0:	lr.d %0, %2\n"				\
			"	bne %0, %z3, 1f\n"			\
			"	sc.d %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			RISCV_ACQUIRE_BARRIER				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_cmpxchg_acquire(ptr, o, n)					\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg_acquire((ptr),			\
					_o_, _n_, sizeof(*(ptr)));	\
})

static inline ulong __cmpxchg_small(void *ptr, ulong old,
				    ulong new, ulong size)
{
	ulong shift;
	ulong ret, mask, temp;
	volatile ulong *ptr32;

	/* Mask inputs to the correct size. */
	mask = GENMASK((size * BITS_PER_BYTE) - 1, 0);
	old &= mask;
	new &= mask;

	/*
	 * Calculate a shift & mask that correspond to the value we wish to
	 * compare & exchange within the naturally aligned 4 byte integer
	 * that includes it.
	 */
	shift = (ulong)ptr & 0x3;
	shift *= BITS_PER_BYTE;
	old <<= shift;
	new <<= shift;
	mask <<= shift;

	/*
	 * Calculate a pointer to the naturally aligned 4 byte integer that
	 * includes our byte of interest, and load its value.
	 */
	ptr32 = (volatile ulong *)((ulong)ptr & ~0x3);

	__asm__ __volatile__ (
		"0:	lr.w %0, %2\n"
		"	and  %1, %0, %z3\n"
		"	bne  %1, %z5, 1f\n"
		"	and  %1, %0, %z4\n"
		"	or   %1, %1, %z6\n"
		"	sc.w.rl %1, %1, %2\n"
		"	bnez %1, 0b\n"
		"	fence w, rw\n"
		"1:\n"
		: "=&r" (ret), "=&r" (temp), "+A" (*ptr32)
		: "rJ" (mask), "rJ" (~mask), "rJ" (old), "rJ" (new)
		: "memory");

	return (ret & mask) >> shift;
}
#define __cmpxchg(ptr, old, new, size)					\
({									\
	__typeof__(ptr) __ptr = (ptr);					\
	__typeof__(*(ptr)) __old = (old);				\
	__typeof__(*(ptr)) __new = (new);				\
	__typeof__(*(ptr)) __ret;					\
	register unsigned int __rc;					\
	switch (size) {							\
	case 1:								\
		__ret = (__typeof__(*(ptr)))				\
			__cmpxchg_small(__ptr, (ulong)__old,		\
					(ulong)__new, (ulong)size);	\
		break;							\
	case 4:								\
		__asm__ __volatile__ (					\
			"0:	lr.w %0, %2\n"				\
			"	bne  %0, %z3, 1f\n"			\
			"	sc.w.rl %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"	fence rw, rw\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" ((long)__old), "rJ" (__new)		\
			: "memory");					\
		break;							\
	case 8:								\
		__asm__ __volatile__ (					\
			"0:	lr.d %0, %2\n"				\
			"	bne %0, %z3, 1f\n"			\
			"	sc.d.rl %1, %z4, %2\n"			\
			"	bnez %1, 0b\n"				\
			"	fence rw, rw\n"				\
			"1:\n"						\
			: "=&r" (__ret), "=&r" (__rc), "+A" (*__ptr)	\
			: "rJ" (__old), "rJ" (__new)			\
			: "memory");					\
		break;							\
	default:							\
		BUILD_BUG();						\
	}								\
	__ret;								\
})

#define arch_cmpxchg(ptr, o, n)						\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg((ptr),				\
				       _o_, _n_, sizeof(*(ptr)));	\
})

#define arch_cmpxchg_local(ptr, o, n)					\
	(__cmpxchg_relaxed((ptr), (o), (n), sizeof(*(ptr))))

#define arch_cmpxchg64(ptr, o, n)					\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_cmpxchg((ptr), (o), (n));					\
})

#define arch_cmpxchg64_local(ptr, o, n)					\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_cmpxchg_relaxed((ptr), (o), (n));				\
})

#endif /* _ASM_RISCV_CMPXCHG_H */
