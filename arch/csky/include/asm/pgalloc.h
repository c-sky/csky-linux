/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ASM_CSKY_PGALLOC_H
#define __ASM_CSKY_PGALLOC_H

#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/sched.h>

static inline void pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmd,
					pte_t *pte)
{
	set_pmd(pmd, __pmd(__pa(pte)));
}

static inline void pmd_populate(struct mm_struct *mm, pmd_t *pmd,
					pgtable_t pte)
{
	set_pmd(pmd, __pmd(__pa(page_address(pte))));
}

#define pmd_pgtable(pmd) pmd_page(pmd)

extern void pgd_init(unsigned long *p);

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm)
{
	pte_t *pte;
	unsigned long i;

	pte = (pte_t *) __get_free_page(GFP_KERNEL);
	if (!pte)
		return NULL;

	for (i = 0; i < PAGE_SIZE/sizeof(pte_t); i++)
		(pte + i)->pte_low = _PAGE_GLOBAL;

	return pte;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm)
{
	struct page *pte;

	pte = alloc_pages(GFP_KERNEL | __GFP_ZERO, 0);
	if (!pte)
		return NULL;

	if (!pgtable_page_ctor(pte)) {
		__free_page(pte);
		return NULL;
	}

	return pte;
}

static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
	free_pages((unsigned long)pte, PTE_ORDER);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	pgtable_page_dtor(pte);
	__free_pages(pte, PTE_ORDER);
}

static inline void pgd_free(struct mm_struct *mm, pgd_t *pgd)
{
	free_pages((unsigned long)pgd, PGD_ORDER);
}

static inline pgd_t *pgd_alloc(struct mm_struct *mm)
{
	pgd_t *ret;
	pgd_t *init;

	ret = (pgd_t *) __get_free_pages(GFP_KERNEL, PGD_ORDER);
	if (ret) {
		init = pgd_offset(&init_mm, 0UL);
		pgd_init((unsigned long *)ret);
		memcpy(ret + USER_PTRS_PER_PGD, init + USER_PTRS_PER_PGD,
			(PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
		/* prevent out of order excute */
		smp_mb();
#ifdef CONFIG_CPU_NEED_TLBSYNC
		dcache_wb_range((unsigned int)ret,
				(unsigned int)(ret + PTRS_PER_PGD));
#endif
	}

	return ret;
}

#define __pte_free_tlb(tlb, pte, address)		\
do {							\
	pgtable_page_dtor(pte);				\
	tlb_remove_page(tlb, pte);			\
} while (0)

#define check_pgt_cache()	do {} while (0)

extern void pagetable_init(void);
extern void pre_mmu_init(void);
extern void pre_trap_init(void);

#endif /* __ASM_CSKY_PGALLOC_H */
