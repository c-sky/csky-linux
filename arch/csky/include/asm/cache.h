#ifndef __ASM_CSKY_CACHE_H
#define __ASM_CSKY_CACHE_H

/* bytes per L1 cache line */
#ifdef	__CSKYABIV1__
#define L1_CACHE_SHIFT	4
#else
#define L1_CACHE_SHIFT	5
#endif
#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)


/* for cr17 */
#define INS_CACHE		(1 << 0)
#define DATA_CACHE		(1 << 1)
#define CACHE_INV		(1 << 4)
#define CACHE_CLR		(1 << 5)
#define CACHE_OMS		(1 << 6)
#define CACHE_ITS		(1 << 7)
#define CACHE_LICF		(1 << 31)

/* for cr22 */
#define CR22_LEVEL_SHIFT        (1)
#define CR22_SET_SHIFT		(7)
#define CR22_WAY_SHIFT          (30)
#define CR22_WAY_SHIFT_L2	(29)

#define ARCH_DMA_MINALIGN	L1_CACHE_BYTES

#ifndef __ASSEMBLY__

#define cache_op_line(i, value) \
	__asm__ __volatile__( \
		"idly4 \n\t" \
		"mtcr	%0, cr22\n\t" \
		"bseti  %1, 6\n\t" \
		"mtcr	%1, cr17\n\t" \
		::"r"(i), "r"(value))

void cache_op_all(
	unsigned int value
	);

void cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value
	);
#endif

#endif  /* __ASM_CSKY_CACHE_H */
