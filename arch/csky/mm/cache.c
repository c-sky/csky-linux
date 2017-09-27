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

#define __cache_op_line(i, value) \
	__asm__ __volatile__( \
		"idly4\n\t" \
		"mtcr	%0, cr22\n\t" \
		"bseti  %1, 6\n\t" \
		"mtcr	%1, cr17\n\t" \
		::"r"(i), "r"(value))


void
cache_op_all(unsigned int value)
{
	__asm__ __volatile__(
		"mtcr	%0, cr17\n\t"
		"sync\n\t"
		::"r"(value));
}

#define FLUSH_MAX PAGE_SIZE

void
cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value
	)
{
	unsigned long i;

	if (unlikely((end - start) >= FLUSH_MAX)) {
		cache_op_all(value | CACHE_CLR);
		return;
	}

	for(i = start; i < end; i += L1_CACHE_BYTES){
		__cache_op_line(i, CACHE_OMS | CACHE_CLR | value);
	}

	__asm__ __volatile__("sync\n\t"::);
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
	cache_op_range(addr, addr + PAGE_SIZE, CACHE_INV|CACHE_CLR|DATA_CACHE);
}

SYSCALL_DEFINE3(cacheflush,
		void __user *, addr,
		unsigned long, bytes,
		int, cache)
{
	switch(cache) {
	case ICACHE:
		cache_op_range(0, FLUSH_MAX, INS_CACHE|
					CACHE_INV);
		break;
	case DCACHE:
		cache_op_range(0, FLUSH_MAX, DATA_CACHE|
					CACHE_CLR|CACHE_INV);
		break;
	case BCACHE:
		cache_op_range(0, FLUSH_MAX, DATA_CACHE|INS_CACHE|
					CACHE_CLR|CACHE_INV);
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
			CACHE_INV);
	}
#endif
	if (vma->vm_flags & VM_EXEC ||
	    pages_do_alias(addr, address & PAGE_MASK))
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);

	clear_bit(PG_arch_1, &(page)->flags);
}

