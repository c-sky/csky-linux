// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/cache.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <asm/cache.h>

void flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	void *va;
	unsigned long start, end;

	va = page_address(page);
	start = (unsigned long) va;

	if (va == NULL && PageHighMem(page))
		start = (unsigned long) kmap_atomic(page);

	end = start + PAGE_SIZE;

	cache_wbinv_range(start, end);

	if (va == NULL && PageHighMem(page))
		kunmap_atomic((void *)start);
}

void update_mmu_cache(struct vm_area_struct *vma, unsigned long address, pte_t *pte)
{
	unsigned long addr, pfn;
	struct page *page;
	void *va;

	if (!(vma->vm_flags & VM_EXEC))
		return;

	pfn = pte_pfn(*pte);
	if (unlikely(!pfn_valid(pfn)))
		return;

	page = pfn_to_page(pfn);
	if (page == ZERO_PAGE(0))
		return;

	va = page_address(page);
	addr = (unsigned long) va;

	if (va == NULL && PageHighMem(page))
		addr = (unsigned long) kmap_atomic(page);

	cache_wbinv_range(addr, addr + PAGE_SIZE);

	if (va == NULL && PageHighMem(page))
		kunmap_atomic((void *) addr);
}

