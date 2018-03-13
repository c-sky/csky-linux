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
#include <abi/reg_ops.h>

static DEFINE_SPINLOCK(cache_lock);

#define __cache_op_line(i, value) \
	asm volatile( \
		"mtcr	%0, cr22\n" \
		"mtcr	%1, cr17\n" \
		::"r"(i), "r"(value))

#define __cache_op_line_atomic(i, value) \
	asm volatile( \
		"idly4\n" \
		"mtcr	%0, cr22\n" \
		"mtcr	%1, cr17\n" \
		"sync\n" \
		::"r"(i), "r"(value))

void
cache_op_line(unsigned int i, unsigned int value)
{
	__cache_op_line_atomic(i, value | CACHE_CLR | CACHE_OMS);
}

#define CCR2_L2E (1 << 3)
void
cache_op_all(unsigned int value, unsigned int l2)
{
	asm volatile(
		"mtcr	%0, cr17\n"
		::"r"(value | CACHE_CLR));

	asm volatile("sync\n");

	if (l2 && (mfcr_ccr2() & CCR2_L2E)) {
		asm volatile(
			"mtcr	%0, cr24\n"
			"sync\n"
			::"r"(value));
	}
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
	bool l2_sync;

	if (unlikely((end - start) >= FLUSH_MAX) ||
	    unlikely(start < PAGE_OFFSET) ||
	    unlikely(start >= PAGE_OFFSET + LOWMEM_LIMIT)) {
		cache_op_all(value, l2);
		return;
	}

	if ((mfcr_ccr2() & CCR2_L2E) && l2)
		l2_sync = 1;
	else
		l2_sync = 0;

	spin_lock_irqsave(&cache_lock, flags);
	for(i = start; i < end; i += L1_CACHE_BYTES) {
		__cache_op_line(i, val);
		if (l2_sync) {
			asm volatile(
				"sync\n");
			asm volatile(
				"mtcr	%0, cr24\n"
				::"r"(val));
		}
	}
	spin_unlock_irqrestore(&cache_lock, flags);

	asm volatile("sync\n");
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

	if (vma->vm_flags & VM_EXEC ||
	    pages_do_alias(addr, address & PAGE_MASK))
		cache_op_all(
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV, 0);

	clear_bit(PG_arch_1, &(page)->flags);
}

