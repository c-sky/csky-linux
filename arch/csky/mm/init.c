#include <linux/bug.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/highmem.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>
#include <linux/pfn.h>

#include <asm/setup.h>
#include <asm/cachectl.h>
#include <asm/dma.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/mmu_context.h>
#include <asm/sections.h>
#include <asm/tlb.h>

#ifndef CONFIG_MMU_HARD_REFILL
unsigned long pgd_current[NR_CPUS];
#endif

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_bss;
pte_t invalid_pte_table[PTRS_PER_PTE] __page_aligned_bss;
unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)] __page_aligned_bss;

#ifndef CONFIG_NEED_MULTIPLE_NODES
static inline int hupage_is_ram(unsigned long pagenr)
{
	if (pagenr >= min_low_pfn && pagenr < max_low_pfn)
		return 1;
	else
		return 0;
}

void __init mem_init(void)
{
	unsigned long codesize, reservedpages, datasize, initsize;
	unsigned long tmp, ram;

#ifdef CONFIG_HIGHMEM
#ifdef CONFIG_DISCONTIGMEM
#error "CONFIG_HIGHMEM and CONFIG_DISCONTIGMEM dont work together yet"
#endif
	max_mapnr = highend_pfn;
#else
	max_mapnr = max_low_pfn;
#endif
	high_memory = (void *) __va(max_low_pfn << PAGE_SHIFT);

	free_all_bootmem();

	reservedpages = ram = 0;
	for (tmp = 0; tmp < max_low_pfn; tmp++)
		if (hupage_is_ram(tmp)) {
			ram++;
			if (PageReserved(pfn_to_page(tmp)))
			        reservedpages++;
		}

#ifdef CONFIG_HIGHMEM
	for (tmp = highstart_pfn; tmp < highend_pfn; tmp++) {
		struct page *page = pfn_to_page(tmp);

		if (!hupage_is_ram(tmp)) {
		        SetPageReserved(page);
		        continue;
		}
		ClearPageReserved(page);
		init_page_count(page);
		__free_page(page);
		totalhigh_pages++;
	}
	totalram_pages += totalhigh_pages;
#endif

	codesize =  (unsigned long) &_etext - (unsigned long) &_stext;
	datasize =  (unsigned long) &_edata - (unsigned long) &_etext;
	initsize =  (unsigned long) &__init_end - (unsigned long) &__init_begin;

	printk(KERN_INFO "Memory: %luk/%luk available (%ldk kernel code, "
	       "%ldk reserved, %ldk data, %ldk init, %ldk highmem)\n",
	       (unsigned long) nr_free_pages() << (PAGE_SHIFT-10),
	       ram << (PAGE_SHIFT-10),
	       codesize >> 10,
	       reservedpages << (PAGE_SHIFT-10),
	       datasize >> 10,
	       initsize >> 10,
	       (unsigned long) (totalhigh_pages << (PAGE_SHIFT-10)));
}
#endif  /* CONFIG_NEED_MULTIPLE_NODES */

#ifdef CONFIG_BLK_DEV_INITRD
void free_initrd_mem(unsigned long start, unsigned long end)
{
	if (start < end)
		printk(KERN_INFO "Freeing initrd memory: %ldk freed\n",
                     (end - start) >> 10);

	for (; start < end; start += PAGE_SIZE) {
	ClearPageReserved(virt_to_page(start));
	init_page_count(virt_to_page(start));
	free_page(start);
	totalram_pages++;
    }
}
#endif

extern char __init_begin[], __init_end[];
extern void __init prom_free_prom_memory(void);

void free_initmem(void)
{
	unsigned long addr;


	addr = (unsigned long) &__init_begin;
	while (addr < (unsigned long) &__init_end) {
	        ClearPageReserved(virt_to_page(addr));
	        init_page_count(virt_to_page(addr));
	        free_page(addr);
	        totalram_pages++;
	        addr += PAGE_SIZE;
	}
	printk(KERN_INFO "Freeing unused kernel memory: %dk freed\n",
	        ((unsigned int)&__init_end - (unsigned int)&__init_begin) >> 10);
}

void pgd_init(unsigned long page)
{
	int i;
	unsigned long *p = (unsigned long *) page;

#ifdef CONFIG_MMU_HARD_REFILL
#define val	(unsigned long) __pa(invalid_pte_table);
#else
#define val	(unsigned long) invalid_pte_table;
#endif

	for (i = 0; i < USER_PTRS_PER_PGD*2; i+=8) {
		p[i + 0] = val;
		p[i + 1] = val;
		p[i + 2] = val;
		p[i + 3] = val;
		p[i + 4] = val;
		p[i + 5] = val;
		p[i + 6] = val;
		p[i + 7] = val;
	}
}

