// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/syscalls.h>
#include <asm/page.h>
#include <asm/cache.h>
#include <asm/cachectl.h>

SYSCALL_DEFINE3(cacheflush,
		void __user *, addr,
		unsigned long, bytes,
		int, cache)
{
	switch(cache) {
	case ICACHE:
		icache_inv_all();
		break;
	case DCACHE:
		dcache_wbinv_all();
		break;
	case BCACHE:
		cache_wbinv_all();
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

	if (vma->vm_flags & VM_EXEC ||
	    pages_do_alias(addr, address & PAGE_MASK))
		dcache_wbinv_all();

	clear_bit(PG_arch_1, &(page)->flags);
}

