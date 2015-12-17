#ifndef __ASM_CSKY_PGTABLE_NOMMU_H
#define __ASM_CSKY_PGTABLE_NOMMU_H

#include <asm/addrspace.h>
#include <asm-generic/4level-fixup.h>

#ifndef __ASSEMBLY__

#include <asm/pgtable-bits.h>

/*
 * This flag is used to indicate that the page pointed to by a pte
 * is dirty and requires cleaning before returning it to the user.
 */
#define PG_dcache_dirty			PG_arch_1

#define Page_dcache_dirty(page)		\
	test_bit(PG_dcache_dirty, &(page)->flags)
#define SetPageDcacheDirty(page)	\
	set_bit(PG_dcache_dirty, &(page)->flags)
#define ClearPageDcacheDirty(page)	\
	clear_bit(PG_dcache_dirty, &(page)->flags)

#define pmd_page_vaddr(pmd)     pmd_val(pmd)

/*
 * Certain architectures need to do special things when pte's
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
#define set_pte(pteptr, pteval)                                 \
        do{                                                     \
                *(pteptr) = (pteval);                           \
        } while(0)
#define set_pte_at(mm,addr,ptep,pteval) set_pte(ptep,pteval)

#endif /* !defined (__ASSEMBLY__) */

/*
 * Basically we have the same two-level (which is the logical three level
 * Linux page table layout folded) page tables as the i386.  Some day
 * when we have proper page coloring support we can have a 1% quicker
 * tlb refill handling mechanism, but for now it is a bit slower but
 * works even with the cache aliasing problem the R4k and above have.
 */

/* PMD_SHIFT determines the size of the area a second-level page table can map*/
#define PMD_SHIFT      22
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))

/* PGDIR_SHIFT determines what a third-level page table entry can map */
#define PGDIR_SHIFT	PMD_SHIFT
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

#define FIRST_USER_ADDRESS      0

/*
 * traditional  two-level paging structure:
 */
#define PGD_ORDER       0
#define PTE_ORDER       0

#define pte_ERROR(e) \
        printk("%s:%d: bad pte %08lx.\n", __FILE__, __LINE__, (e).pte_low)
#define pmd_ERROR(e) \
        printk("%s:%d: bad pmd %08lx.\n", __FILE__, __LINE__, pmd_val(e))
#define pgd_ERROR(e) \
        printk("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))

#define VMALLOC_START     	CK_RAM_BASE
#define VMALLOC_END       	CK_RAM_END

#define PAGE_NONE       __pgprot(0)
#define PAGE_SHARED     __pgprot(0)
#define PAGE_COPY       __pgprot(0)
#define PAGE_READONLY   __pgprot(0)
#define PAGE_KERNEL     __pgprot(0)
#define PAGE_USERIO     __pgprot(0)
#define PAGE_KERNEL_UNCACHED __pgprot(0)

#define __swp_type(x)           (0)
#define __swp_offset(x)         (0)
#define __swp_entry(typ,off)    ((swp_entry_t) { ((typ) | ((off) << 7)) })
#define __pte_to_swp_entry(pte) ((swp_entry_t) { pte_val(pte) })
#define __swp_entry_to_pte(x)   ((pte_t) { (x).val })

#define ZERO_PAGE(vaddr)        (virt_to_page(0))
#define swapper_pg_dir ((pgd_t *) 0)

void load_pgd(unsigned long pg_dir);

extern pmd_t invalid_pte_table[PAGE_SIZE/sizeof(pmd_t)];

static inline int pte_special(pte_t pte)        { return 0; }
static inline pte_t pte_mkspecial(pte_t pte)    { return pte; }

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
#define pmd_phys(pmd)           virt_to_phys((void *)pmd_val(pmd))
#define pmd_page(pmd)           (pfn_to_page(pmd_phys(pmd) >> PAGE_SHIFT))
#define pmd_page_vaddr(pmd)     pmd_val(pmd)

static inline void pmd_set(pmd_t * pmdp, pte_t * ptep)
{
	pmd_val(*pmdp) = (((unsigned long) ptep) & PAGE_MASK);
}


/*
 * (pmds are folded into pgds so this doesn't get actually called,
 * but the define is needed for a generic inline function.)
 */
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)
#define set_pgd(pgdptr, pgdval) (*(pgdptr) = pgdval)

/*
 * Empty pgd/pmd entries point to the invalid_pte_table.
 */
static inline int pmd_none(pmd_t pmd)
{
	return pmd_val(pmd) == (unsigned long) invalid_pte_table;
}

#define pmd_bad(pmd)        (pmd_val(pmd) & ~PAGE_MASK)

static inline int pmd_present(pmd_t pmd)
{
	return (pmd_val(pmd) != (unsigned long) invalid_pte_table);
}

static inline void pmd_clear(pmd_t *pmdp)
{
	pmd_val(*pmdp) = ((unsigned long) invalid_pte_table);
}

/*
 * The "pgd_xxx()" functions here are trivial for a folded two-level
 * setup: the pgd is never bad, and a pmd always exists (as it's folded
 * into the pgd entry)
 */
#define  pgd_none(pgd)                (0)
#define  pgd_bad(pgd)                (0)
static inline int pgd_present(pgd_t pgd)	{ return 1; }
static inline void pgd_clear(pgd_t *pgdp)	{ }

/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */
static inline int pte_read(pte_t pte)
{
    return (pte).pte_low & _PAGE_READ;
}

static inline int pte_write(pte_t pte)
{
    return (pte).pte_low & _PAGE_WRITE;
}

static inline int pte_dirty(pte_t pte)
{
    return (pte).pte_low & _PAGE_MODIFIED;
}

static inline int pte_young(pte_t pte)
{
    return (pte).pte_low & _PAGE_ACCESSED;
}

static inline int pte_file(pte_t pte)
{
    return pte_val(pte) & _PAGE_FILE;
}

static inline pte_t pte_wrprotect(pte_t pte)
{
    pte_val(pte) &= ~(_PAGE_WRITE | _PAGE_SILENT_WRITE);
    return pte;
}

static inline pte_t pte_mkclean(pte_t pte)
{
    pte_val(pte) &= ~(_PAGE_MODIFIED|_PAGE_SILENT_WRITE);
    return pte;
}

static inline pte_t pte_mkold(pte_t pte)
{
    pte_val(pte) &= ~(_PAGE_ACCESSED|_PAGE_SILENT_READ);
    return pte;
}

static inline pte_t pte_mkwrite(pte_t pte)
{
    pte_val(pte) |= _PAGE_WRITE;
    if (pte_val(pte) & _PAGE_MODIFIED)
        pte_val(pte) |= _PAGE_SILENT_WRITE;
    return pte;
}

static inline pte_t pte_mkdirty(pte_t pte)
{
    pte_val(pte) |= _PAGE_MODIFIED;
    if (pte_val(pte) & _PAGE_WRITE)
        pte_val(pte) |= _PAGE_SILENT_WRITE;
    return pte;
}

static inline pte_t pte_mkyoung(pte_t pte)
{
    pte_val(pte) |= _PAGE_ACCESSED;
    if (pte_val(pte) & _PAGE_READ)
        pte_val(pte) |= _PAGE_SILENT_READ;
    return pte;
}

#define PGD_T_LOG2	ffz(~sizeof(pgd_t))
#define PMD_T_LOG2	ffz(~sizeof(pmd_t))
#define PTE_T_LOG2	ffz(~sizeof(pte_t))

#define PTRS_PER_PGD	((PAGE_SIZE << PGD_ORDER) / sizeof(pgd_t))
#define PTRS_PER_PMD	1
#define PTRS_PER_PTE	((PAGE_SIZE << PTE_ORDER) / sizeof(pte_t))

#define __pgd_offset(address)	pgd_index(address)
#define __pmd_offset(address) \
	(((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

#define pgd_index(address)	((address) >> PGDIR_SHIFT)

/*
 * Macro to make mark a page protection value as "uncacheable".  Note
 * that "protection" is really a misnomer here as the protection value
 * contains the memory attribute bits, dirty bits, and various other
 * bits as well.
 */
#define pgprot_noncached pgprot_noncached


static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
       return _prot;
}

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
#define mk_pte(page, pgprot)    pfn_pte(page_to_pfn(page), (pgprot))
static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
        return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}

/* to find an entry in a page-table-directory */
static inline pgd_t *pgd_offset(struct mm_struct *mm, unsigned long address)
{
	return mm->pgd + pgd_index(address);
}

/* Find an entry in the second-level page table.. */
static inline pmd_t *pmd_offset(pgd_t *dir, unsigned long address)
{
	return (pmd_t *) dir;
}

/* Find an entry in the third-level page table.. */
static inline pte_t *pte_offset(pmd_t * dir, unsigned long address)
{
	return (pte_t *) (pmd_page_vaddr(*dir)) +
	       ((address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1));
}

extern void __update_tlb(struct vm_area_struct *vma, unsigned long address,
	pte_t pte);
extern void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte);
extern void show_jtlb_table(void);

static inline void update_mmu_cache(struct vm_area_struct *vma,
	unsigned long address, pte_t *ptep)
{
	pte_t pte = *ptep;
	__update_tlb(vma, address, pte);
	__update_cache(vma, address, pte);
}

extern void paging_init(void);

/*
*Needs to be defined here and not in linux/mm.h, as it is arch dependent
*/
#define PageSkip(page)		(0)
#define kern_addr_valid(addr)	(1)

#include <asm-generic/pgtable.h>
#endif /* ifndef (__ASSEMBLY__) */

#include <asm-generic/pgtable.h>
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)

/*
 * No page table caches to initialise
 */
#define pgtable_cache_init()	do { } while (0)

#define io_remap_pfn_range(vma, vaddr, pfn, size, prot)         \
                remap_pfn_range(vma, vaddr, pfn, size, prot)

/* provide a special get_unmapped_area for framebuffer mmaps of nommu */
extern unsigned long get_fb_unmapped_area(struct file *filp, unsigned long,
					  unsigned long, unsigned long,
					  unsigned long);
#define HAVE_ARCH_FB_UNMAPPED_AREA

#endif /* __ASM_CSKY_PGTABLE_NOMMU_H */
