#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/cache.h>

#ifdef CONFIG_CSKY_INSTRUCTION_CACHE
#define dis_icache(tmp) \
	__asm__ __volatile__( \
		"mfcr %0, cr18\n\t" \
		"bclri %0, 2\n\t" \
		"mtcr %0, cr18\n\t" \
		::"r"(tmp))

#define en_icache(tmp) \
	__asm__ __volatile__( \
		"mfcr %0, cr18\n\t" \
		"bseti %0, 2\n\t" \
		"mtcr %0, cr18\n\t" \
		::"r"(tmp))
#else
#define dis_icache(tmp) do{}while(0)
#define en_icache(tmp) do{}while(0)
#endif

#define set_cr17(value) \
	__asm__ __volatile__( \
		"mtcr	%0, cr17\n\t" \
		::"r"(value))

inline void
cache_op_all(unsigned int value)
{
	unsigned long flags = 0;
	unsigned int tmp = 0;

	if (unlikely(value & INS_CACHE)) {
		local_irq_save(flags);
		dis_icache(tmp);
	}

	set_cr17(value);

	if (unlikely(value & INS_CACHE)) {
		en_icache(tmp);
		local_irq_restore(flags);
	}
}


#define set_cr22(value) \
	__asm__ __volatile__( \
		"mtcr	%0, cr22\n\t" \
		::"r"(value))
inline void
cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value
	)
{
#ifndef CONFIG_CSKY_CACHE_LINE_FLUSH
	cache_op_all(value);
#else /* CONFIG_CSKY_CACHE_LINE_FLUSH */
	unsigned long i,flags;
	unsigned int tmp = 0;

	if (unlikely((start & 0xf0000000) !=
		(CONFIG_RAM_BASE + PAGE_OFFSET - PHYS_OFFSET))) {
		cache_op_all(value);
		return;
	}

	if (unlikely((end - start) > PAGE_SIZE)) {
		cache_op_all(value);
		return;
	}

	local_irq_save(flags);

	if (unlikely(value & INS_CACHE))
		dis_icache(tmp);

	for(i = start; i < end; i += L1_CACHE_BYTES){
		set_cr22(i);
		set_cr17(CACHE_OMS | value);
	}

	if (unlikely(end & (L1_CACHE_BYTES-1))) {
		set_cr22(end);
		set_cr17(CACHE_OMS | value);
	}

	if (unlikely(value & INS_CACHE))
		en_icache(tmp);

	local_irq_restore(flags);
#endif /* CONFIG_CSKY_CACHE_LINE_FLUSH */
}

