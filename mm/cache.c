/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 */
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/cachectl.h>

void flush_data_cache_page(unsigned long addr)
{ 
        int value = 0x32;

        __asm__ __volatile__("sync\n\t"
                             "mtcr %0,cr17\n\t"
                             : :"r" (value));
}

asmlinkage int sys_cacheflush(void __user *addr, unsigned long bytes, int cache)
{
	if (bytes == 0)
		return 0;
	if (!access_ok(VERIFY_WRITE, addr, bytes))
		return -EFAULT;

	switch(cache) {
	case ICACHE:
		flush_icache_range((unsigned long)addr, 
			(unsigned long)addr + bytes);
		break;
	case DCACHE:
		flush_dcache_all();
		break;
	case BCACHE:
		/* This should flush more selectivly ...  */
		flush_cache_all();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

void flush_dcache_page(struct page *page)
{
	struct address_space *mapping = page_mapping(page);
	unsigned long addr;

#if defined (CONFIG_CPU_CSKYV2)
	if (PageHighMem(page))
		return;
#endif
	if (mapping && !mapping_mapped(mapping)) {
		SetPageDcacheDirty(page);
		return;
	}

	/*
	 * We could delay the flush for the !page_mapping case too.  But that
	 * case is for exec env/arg pages and those are %99 certainly going to
	 * get faulted into the tlb (and thus flushed) anyways.
	 */
	addr = (unsigned long) page_address(page);
	flush_data_cache_page(addr);
}

void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte)
{
	unsigned long addr;
#if defined (CONFIG_MMU)
	struct page *page;
	unsigned long pfn;
	int exec = vma->vm_flags & VM_EXEC;

	pfn = pte_pfn(pte);
	if (unlikely(!pfn_valid(pfn)))
		return;
	page = pfn_to_page(pfn);
//	if (page_mapping(page) && Page_dcache_dirty(page)) {
		addr = (unsigned long) page_address(page);
#if defined (CONFIG_HIGHMEM) && defined (CONFIG_CPU_CSKYV1)
		if (PageHighMem(page)){
			flush_data_cache_page(addr);
		}
#endif
		if (exec || pages_do_alias(addr, address & PAGE_MASK))
			flush_data_cache_page(addr);
		ClearPageDcacheDirty(page);
//	}
#else
	flush_data_cache_page(addr);
#endif
}

