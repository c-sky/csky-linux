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
#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/cachectl.h>

asmlinkage int sys_cacheflush(void __user *addr, unsigned long bytes, int cache)
{
	unsigned int start;
	unsigned int end;

	if (bytes == 0)
		return 0;

	if (!access_ok(VERIFY_WRITE, addr, bytes))
		return -EFAULT;

	start = (unsigned int)addr;
	end = start + bytes;

	switch(cache) {
	case ICACHE:
		cache_op_range(
			start, end,
			INS_CACHE|
			CACHE_INV);
		break;
	case DCACHE:
		cache_op_range(
			start, end,
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
		break;
	case BCACHE:
		cache_op_range(
			start, end,
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
	int exec = vma->vm_flags & VM_EXEC;

	pfn = pte_pfn(pte);
	if (unlikely(!pfn_valid(pfn)))
		return;
	page = pfn_to_page(pfn);
		addr = (unsigned long) page_address(page);
#if defined (CONFIG_HIGHMEM) && defined (CONFIG_CPU_CSKYV1)
	if (PageHighMem(page)){
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
	}
#endif
	if (exec || pages_do_alias(addr, address & PAGE_MASK))
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);

	clear_bit(PG_arch_1, &(page)->flags);
}

