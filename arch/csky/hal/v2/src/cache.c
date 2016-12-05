#include <asm/page.h>

inline void
cache_op_all(unsigned int value)
{
	__asm__ __volatile__(
		"mtcr	%0, cr17\n\t"
		"sync\n\t"
		::"r"(value));
}

#define cache_op_line(i, value) \
	__asm__ __volatile__( \
		"idly4 \n\t" \
		"mtcr	%0, cr22\n\t" \
		"mtcr	%1, cr17\n\t" \
		::"r"(i), "r"(value))

inline void
cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value
	)
{
	unsigned long i;

	if (unlikely(start < PAGE_OFFSET)) {
		cache_op_all(value | CACHE_CLR);
		return;
	}

	if (unlikely((end - start) > PAGE_SIZE)) {
		cache_op_all(value | CACHE_CLR);
		return;
	}

	for(i = start; i < end; i += L1_CACHE_BYTES){
		cache_op_line(i, CACHE_OMS | CACHE_CLR | value);
	}

	if (unlikely(end & (L1_CACHE_BYTES-1))) {
		cache_op_line(end, CACHE_OMS | CACHE_CLR | value);
	}

	__asm__ __volatile__("sync\n\t"::);
}

