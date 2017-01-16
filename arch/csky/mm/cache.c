#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/cachectl.h>

inline void
cache_op_all(unsigned int value)
{
	__asm__ __volatile__(
		"mtcr	%0, cr17\n\t"
		"sync\n\t"
		::"r"(value));
}

inline void
cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value
	)
{
	unsigned long i;

	if (unlikely((end - start) > PAGE_SIZE*2)) {
		cache_op_all(value | CACHE_CLR);
		return;
	}

	for(i = start; i < end; i += L1_CACHE_BYTES){
		cache_op_line(i, CACHE_OMS | CACHE_CLR | value);
	}

	__asm__ __volatile__("sync\n\t"::);
}

SYSCALL_DEFINE3(cacheflush,
		void __user *, addr,
		unsigned long, bytes,
		int, cache)
{
	switch(cache) {
	case ICACHE:
		cache_op_all(
			INS_CACHE|
			CACHE_INV);
		break;
	case DCACHE:
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
		break;
	case BCACHE:
		cache_op_all(
			INS_CACHE|
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
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

