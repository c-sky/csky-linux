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

/* for L1-cache */
#define INS_CACHE		(1 << 0)
#define DATA_CACHE		(1 << 1)
#define CACHE_INV		(1 << 4)
#define CACHE_CLR		(1 << 5)
#define CACHE_OMS		(1 << 6)
#define CACHE_ITS		(1 << 7)
#define CACHE_LICF		(1 << 31)

/* for L2-cache */
#define CR22_LEVEL_SHIFT        (1)
#define CR22_SET_SHIFT		(7)
#define CR22_WAY_SHIFT          (30)
#define CR22_WAY_SHIFT_L2	(29)

static DEFINE_SPINLOCK(cache_lock);

#define cache_op_line(i, value) \
	asm volatile( \
		"mtcr	%0, cr22\n" \
		"mtcr	%1, cr17\n" \
		::"r"(i), "r"(value))

#define cache_op_line_atomic(i, value) \
	asm volatile( \
		"idly4\n" \
		"mtcr	%0, cr22\n" \
		"mtcr	%1, cr17\n" \
		"sync\n" \
		::"r"(i), "r"(value))

#define CCR2_L2E (1 << 3)
static void cache_op_all(unsigned int value, unsigned int l2)
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

static void cache_op_range(
	unsigned int start,
	unsigned int end,
	unsigned int value,
	unsigned int l2)
{
	unsigned long i, flags;
	unsigned int val = value | CACHE_CLR | CACHE_OMS;
	bool l2_sync;

	if (unlikely((end - start) >= PAGE_SIZE) ||
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
		cache_op_line(i, val);
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

void inline dcache_wb_line(unsigned long start)
{
	cache_op_line_atomic(start, DCACHE|CACHE_CLR);
}

void icache_inv_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, INS_CACHE|CACHE_INV, 0);
}

void icache_inv_all(void)
{
	cache_op_all(INS_CACHE|CACHE_INV, 0);
}

void dcache_wb_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, DATA_CACHE|CACHE_CLR, 0);
}

void dcache_wbinv_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, DATA_CACHE|CACHE_CLR|CACHE_INV, 0);
}

void dcache_inv_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, DATA_CACHE|CACHE_INV, 0);
}

void dcache_wbinv_all(void)
{
	cache_op_all(DATA_CACHE|CACHE_CLR|CACHE_INV, 0);
}

void cache_wbinv_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0);
}

void cache_wbinv_all(void)
{
	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0);
}

void dma_wbinv_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, DATA_CACHE|CACHE_CLR|CACHE_INV, 1);
}

void dma_wb_range(unsigned long start, unsigned long end)
{
	cache_op_range(start, end, DATA_CACHE|CACHE_INV, 1);
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> */
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

