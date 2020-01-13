// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
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
#include <linux/memblock.h>
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

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned_bss;
pte_t invalid_pte_table[PTRS_PER_PTE] __page_aligned_bss;
EXPORT_SYMBOL(invalid_pte_table);
unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)] __page_aligned_bss;

void __init mem_init(void)
{
#ifdef CONFIG_HIGHMEM
	unsigned long tmp;
	max_mapnr = highend_pfn;
#else
	max_mapnr = max_low_pfn;
#endif
	high_memory = (void *) __va(max_low_pfn << PAGE_SHIFT);

	free_all_bootmem();

#ifdef CONFIG_HIGHMEM
	for (tmp = highstart_pfn; tmp < highend_pfn; tmp++) {
		struct page *page = pfn_to_page(tmp);

		/* FIXME not sure about */
		if (!memblock_is_reserved(tmp << PAGE_SHIFT))
			free_highmem_page(page);
	}
#endif
	mem_init_print_info(NULL);
}

#ifdef CONFIG_BLK_DEV_INITRD
void free_initrd_mem(unsigned long start, unsigned long end)
{
	if (start < end)
		pr_info("Freeing initrd memory: %ldk freed\n",
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
	pr_info("Freeing unused kernel memory: %dk freed\n",
	        ((unsigned int)&__init_end - (unsigned int)&__init_begin) >> 10);
}

void pgd_init(unsigned long *p)
{
	int i;
	for (i = 0; i<PTRS_PER_PGD; i++)
		p[i] = __pa(invalid_pte_table);
}

void __init pre_mmu_init(void)
{
	/*
	 * Setup page-table and enable TLB-hardrefill
	 */
	flush_tlb_all();
	pgd_init((unsigned long *)swapper_pg_dir);
	TLBMISS_HANDLER_SETUP_PGD(swapper_pg_dir);
	TLBMISS_HANDLER_SETUP_PGD_KERNEL(swapper_pg_dir);

	/* Setup page mask to 4k */
	write_mmu_pagemask(0);
}

void __init fixrange_init(unsigned long start, unsigned long end,
			pgd_t *pgd_base)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	int i, j, k;
	unsigned long vaddr;

	vaddr = start;
	i = __pgd_offset(vaddr);
	j = __pud_offset(vaddr);
	k = __pmd_offset(vaddr);
	pgd = pgd_base + i;

	for ( ; (i < PTRS_PER_PGD) && (vaddr != end); pgd++, i++) {
		pud = (pud_t *)pgd;
		for ( ; (j < PTRS_PER_PUD) && (vaddr != end); pud++, j++) {
			pmd = (pmd_t *)pud;
			for (; (k < PTRS_PER_PMD) && (vaddr != end); pmd++, k++) {
				if (pmd_none(*pmd)) {
					pte = (pte_t *) alloc_bootmem_low_pages(PAGE_SIZE);
					if (!pte)
						panic("%s: Failed to allocate %lu bytes align=%lx\n",
						      __func__, PAGE_SIZE,
						      PAGE_SIZE);

					set_pmd(pmd, __pmd(__pa(pte)));
					BUG_ON(pte != pte_offset_kernel(pmd, 0));
				}
				vaddr += PMD_SIZE;
			}
			k = 0;
		}
		j = 0;
	}
}

void __init fixaddr_init(void)
{
	unsigned long vaddr;

	vaddr = __fix_to_virt(__end_of_fixed_addresses - 1) & PMD_MASK;
	fixrange_init(vaddr, vaddr + PMD_SIZE, swapper_pg_dir);
}
