#ifndef __ASM_CSKY_CACHE_H
#define __ASM_CSKY_CACHE_H

/* bytes per L1 cache line */
#ifdef	CONFIG_CPU_CSKYV1
#define    L1_CACHE_SHIFT    4		/* 16 Bytes */
#else
#define    L1_CACHE_SHIFT    5		/* 32 Bytes */
#define    L2_CACHE_SHIFT    5		/* 32 Bytes */
#define    L2_CACHE_BYTES    (1 << L2_CACHE_SHIFT)
#endif

/* this need to be at least 1 */
#define    L1_CACHE_BYTES    (1 << L1_CACHE_SHIFT)

#define __cacheline_aligned	__attribute__((aligned(L1_CACHE_BYTES)))
#define ____cacheline_aligned	__attribute__((aligned(L1_CACHE_BYTES)))

/* for cr17 */
#define INS_CACHE       (1 << 0)
#define DATA_CACHE      (1 << 1)
#define CACHE_INV       (1 << 4)
#define CACHE_CLR       (1 << 5)
#define CACHE_OMS       (1 << 6)
#define CACHE_ITS       (1 << 7)
#define CACHE_LICF	(1 << 31)

/* for cr22 */
#define CR22_LEVEL_SHIFT        (1)
#define CR22_SET_SHIFT		(7)
#define CR22_WAY_SHIFT          (30)
#define CR22_WAY_SHIFT_L2	(29)

/* for cr24 */
#define L2_CACHE_INV    (1 << 4)
#define L2_CACHE_CLR    (1 << 5)
#define L2_CACHE_OMS    (1 << 6)
#define L2_CACHE_ITS    (1 << 7)
#define L2_CACHE_LICF   (1 << 31)

/*
 * Memory returned by kmalloc() may be used for DMA, so we must make
 * sure that all such allocations are cache aligned. Otherwise,
 * unrelated code may cause parts of the buffer to be read into the
 * cache before the transfer is done, causing old data to be seen by
 * the CPU.
 */
#define ARCH_DMA_MINALIGN	L1_CACHE_BYTES
#define SMP_CACHE_BYTES		L1_CACHE_BYTES
#define ARCH_SLAB_MINALIGN	8

#ifndef __ASSEMBLY__
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
