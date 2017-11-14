#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/cachectl.h>
#include <hal/reg_ops.h>

static DEFINE_SPINLOCK(cache_lock);
static unsigned int l2_enable = 0;

void
cache_op_l2enable(void)
{
	l2_enable = 1;
}

#define __cache_op_line(i, value) \
	__asm__ __volatile__( \
		"mtcr	%0, cr22\n\t" \
		"mtcr	%1, cr17\n\t" \
		::"r"(i), "r"(value))

void
cache_op_line(unsigned int i, unsigned int value)
{
	unsigned long flags;
	spin_lock_irqsave(&cache_lock, flags);
	__cache_op_line(i, value | CACHE_CLR | CACHE_OMS);
	spin_unlock_irqrestore(&cache_lock, flags);

	if (l2_enable)
		L1_SYNC;
	else
		__asm__ __volatile__("sync\n\t");
}

void
cache_op_all(unsigned int value, unsigned int l2)
{
	__asm__ __volatile__(
		"mtcr	%0, cr17\n\t"
		::"r"(value | CACHE_CLR));

	if (l2_enable) {
		L1_SYNC;
		if (l2) goto flush_l2;
	} else
		__asm__ __volatile__("sync\n\t");

	return;
flush_l2:
	__asm__ __volatile__(
		"mtcr	%0, cr24\n\t"
		"sync\n\t"
		::"r"(value));
}

#define FLUSH_MAX PAGE_SIZE

void
cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value,
	unsigned int l2)
{
	unsigned long i, flags;
	unsigned int val = value | CACHE_CLR | CACHE_OMS;

	if (unlikely((end - start) >= FLUSH_MAX) ||
	    unlikely(start < PAGE_OFFSET) ||
	    unlikely(start >= V3GB_OFFSET)) {
		cache_op_all(value, l2);
		return;
	}


	for(i = start; i < end; i += L1_CACHE_BYTES) {
		spin_lock_irqsave(&cache_lock, flags);
		__cache_op_line(i, val);
		if (l2_enable) {
			L1_SYNC;
			if (l2) __asm__ __volatile__(
				"mtcr	%0, cr24\n\t"
				::"r"(val));
		}
		spin_unlock_irqrestore(&cache_lock, flags);
	}

	if(l2_enable && !l2) return;

	__asm__ __volatile__("sync\n\t");
}

void flush_dcache_page(struct page *page)
{
	struct address_space *mapping = page_mapping(page);
	unsigned long addr;

	if (mapping && !mapping_mapped(mapping)) {
		set_bit(PG_arch_1, &(page)->flags);
		return;
	}

	/*
	 * We could delay the flush for the !page_mapping case too.  But that
	 * case is for exec env/arg pages and those are %99 certainly going to
	 * get faulted into the tlb (and thus flushed) anyways.
	 */
	addr = (unsigned long) page_address(page);
	cache_op_range(addr, addr + PAGE_SIZE, CACHE_INV|CACHE_CLR|DATA_CACHE, 0);
}

SYSCALL_DEFINE3(cacheflush,
		void __user *, addr,
		unsigned long, bytes,
		int, cache)
{
	switch(cache) {
	case ICACHE:
		cache_op_range(0, FLUSH_MAX, INS_CACHE|
					CACHE_INV, 0);
		break;
	case DCACHE:
		cache_op_range(0, FLUSH_MAX, DATA_CACHE|
					CACHE_CLR|CACHE_INV, 0);
		break;
	case BCACHE:
		cache_op_range(0, FLUSH_MAX, DATA_CACHE|INS_CACHE|
					CACHE_CLR|CACHE_INV, 0);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte)
{
	unsigned long addr;
	struct page *page;
	unsigned long pfn;

	pfn = pte_pfn(pte);
	if (unlikely(!pfn_valid(pfn)))
		return;

	page = pfn_to_page(pfn);
	addr = (unsigned long) page_address(page);

/* gary? */
#if defined (CONFIG_HIGHMEM) && defined (__CSKYABIV1__)
	if (PageHighMem(page)){
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV, 0);
	}
#endif
	if (vma->vm_flags & VM_EXEC ||
	    pages_do_alias(addr, address & PAGE_MASK))
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV, 0);

	clear_bit(PG_arch_1, &(page)->flags);
}

