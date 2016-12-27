#ifndef __ASM_CSKY_PGALLOC_H
#define __ASM_CSKY_PGALLOC_H

#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/sched.h>

#ifdef CONFIG_MMU_HARD_REFILL
/* hard refill need fill PA. */
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
#else
static inline void pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmd,
        pte_t *pte)
{
        set_pmd(pmd, __pmd((unsigned long)pte));
}

static inline void pmd_populate(struct mm_struct *mm, pmd_t *pmd,
        pgtable_t pte)
{
        set_pmd(pmd, __pmd((unsigned long)page_address(pte)));
}
#endif

#define pmd_pgtable(pmd) pmd_page(pmd)

/*
 * Initialize new page directory with pointers to invalid ptes
 */
extern void pgd_init(unsigned long page);

static inline pte_t *pte_alloc_one_kernel(struct mm_struct *mm,
        unsigned long address)
{
        pte_t *pte;

        pte = (pte_t *) __get_free_pages(GFP_KERNEL | __GFP_REPEAT
                              | __GFP_ZERO, PTE_ORDER);

        return pte;
}

static inline struct page *pte_alloc_one(struct mm_struct *mm,
                                            unsigned long address)
{
	struct page *pte;

        pte = alloc_pages(GFP_KERNEL | __GFP_REPEAT, PTE_ORDER);
        if (pte) {
                clear_highpage(pte);
                pgtable_page_ctor(pte);
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
        pgd_t *ret, *init;

        ret = (pgd_t *) __get_free_pages(GFP_KERNEL, PGD_ORDER);
        if (ret) {
                init = pgd_offset(&init_mm, 0UL);
                pgd_init((unsigned long)ret);
                memcpy(ret + USER_PTRS_PER_PGD, init + USER_PTRS_PER_PGD,
                       (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
        }

#ifdef CONFIG_MMU_HARD_REFILL
	cache_op_all(DATA_CACHE|CACHE_CLR);
#endif

        return ret;
}

#define __pte_free_tlb(tlb,pte,address)                 \
do {                                                    \
        pgtable_page_dtor(pte);                         \
        tlb_remove_page((tlb), pte);                    \
} while (0)

#define check_pgt_cache()               do {} while(0)

extern void pagetable_init(void);

#endif /* __ASM_CSKY_PGALLOC_H */

