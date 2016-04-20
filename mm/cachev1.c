#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>

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

static inline void
cache_clr_inv_all(unsigned int value)
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
__flush_cache_range(
		unsigned long start,
		unsigned long end,
		unsigned long value)
{
#ifndef CONFIG_CSKY_CACHE_LINE_FLUSH
	cache_clr_inv_all(value);
#else
	unsigned long i,flags;
	unsigned int tmp = 0;

	if (unlikely((start & 0xf0000000) !=
		(CK_RAM_BASE + PAGE_OFFSET - PHYS_OFFSET))) {
		cache_clr_inv_all(value);
		return;
	}

	if (unlikely((end - start) > PAGE_SIZE)) {
		cache_clr_inv_all(value);
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
#endif /* CSKY_CACHE_LINE_FLUSH */
}

/* just compat for misc, don't care following code.*/
void
__flush_dcache_range(
	unsigned long start,
	unsigned long end
	)
{
	__flush_cache_range(
		start, end,
		DATA_CACHE|CACHE_INV|CACHE_CLR);
}

void
__flush_icache_range(
	unsigned long start,
	unsigned long end
	)
{
	__flush_cache_range(
		start, end,
		INS_CACHE|CACHE_INV|CACHE_CLR);
}

void
__flush_all_range(
	unsigned long start,
	unsigned long end
	)
{
	__flush_cache_range(
		start, end,
		INS_CACHE|DATA_CACHE|
		CACHE_INV|CACHE_CLR);
}


void _flush_cache_all(void)
{
	cache_clr_inv_all(0x33);
}

void ___flush_cache_all(void)
{
	cache_clr_inv_all(0x33);
}

void _flush_cache_mm(struct mm_struct *mm)
{
	cache_clr_inv_all(0x33);
}

void _flush_cache_page(struct vm_area_struct *vma, unsigned long page)
{
	cache_clr_inv_all(0x33);
}

void _flush_cache_range(
		struct vm_area_struct *mm,
		unsigned long start,
		unsigned long end
		)
{
	cache_clr_inv_all(0x33);
}

void _flush_cache_sigtramp(unsigned long addr)
{
	cache_clr_inv_all(0x33);
}

void _flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	cache_clr_inv_all(0x11);
}

void _flush_icache_all(void)
{
	cache_clr_inv_all(0x11);
}

void _flush_icache_range(unsigned long start, unsigned long end)
{
	cache_clr_inv_all(0x11);
}

void _flush_dcache_page(struct page * page)
{
	cache_clr_inv_all(0x32);
}

void _flush_dcache_all(void)
{
	cache_clr_inv_all(0x32);
}

void _clear_dcache_all(void)
{
	cache_clr_inv_all(0x22);
}

void _clear_dcache_range(unsigned long start, unsigned long end)
{
	cache_clr_inv_all(0x22);
}

