#ifndef __ASM_CSKY_PAGE_H
#define __ASM_CSKY_PAGE_H

#include <asm/setup.h>
#include <asm/cache.h>
#include <linux/const.h>

/*
 * PAGE_SHIFT determines the page size
 */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(_AC(1, UL) << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define THREAD_SIZE	(PAGE_SIZE * 2)

/*
 * NOTE: virtual isn't really correct, actually it should be the offset into the
 * memory node, but we have no highmem, so that works for now.
 * TODO: implement (fast) pfn<->pgdat_idx conversion functions, this makes lots
 * of the shifts unnecessary.
 */

#ifndef __ASSEMBLY__

#include <linux/pfn.h>

#define PHY_OFFSET		CONFIG_RAM_BASE
#define ARCH_PFN_OFFSET		PFN_UP(PHY_OFFSET)

#define virt_to_pfn(kaddr)      (__pa(kaddr) >> PAGE_SHIFT)
#define pfn_to_virt(pfn)        __va((pfn) << PAGE_SHIFT)

#define virt_addr_valid(kaddr)  ((void *)(kaddr) >= (void *)PAGE_OFFSET && \
                                 (void *)(kaddr) < high_memory)
extern unsigned long min_low_pfn, max_pfn;
#define pfn_valid(pfn)		((pfn >= min_low_pfn) && (pfn < max_pfn))

extern void *memset(void *dest, int c, size_t l);
extern void *memcpy (void *to, const void *from, size_t l);

#define clear_page(page)        memset((page), 0, PAGE_SIZE)
#define copy_page(to,from)      memcpy((to), (from), PAGE_SIZE)

#define page_to_phys(page)	(page_to_pfn(page) << PAGE_SHIFT)

extern unsigned long shm_align_mask;

static inline unsigned long pages_do_alias(unsigned long addr1,
	unsigned long addr2)
{
	return (addr1 ^ addr2) & shm_align_mask;
}

struct page;

#include <hal/page.h>

struct vm_area_struct;

/*
 * These are used to make use of C type-checking..
 */
typedef struct { unsigned long pte_low; } pte_t;
#define pte_val(x)    ((x).pte_low)

typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;
typedef struct page *pgtable_t;

#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define ptep_buddy(x)	((pte_t *)((unsigned long)(x) ^ sizeof(pte_t)))

#define __pte(x)	((pte_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

#endif /* !__ASSEMBLY__ */

/*
 * This handles the memory map.
 * We handle pages at KSEG0 for kernels with 32 bit address space.
 */

#define	PAGE_OFFSET	0x80000000
#define	V3GB_OFFSET	0xc0000000

#define LOWMEM_LIMIT	0x20000000

#ifdef CONFIG_PHYSICAL_BASE_CHANGE
#define PHYS_OFFSET     CONFIG_SSEG0_BASE
#else
#define PHYS_OFFSET     0x0
#endif

#define UNCACHE_BASE	0xa0000000
#define UNCACHE_MASK	0x1fffffff

#define __pa(x)		(((unsigned long) (x) - PAGE_OFFSET + PHYS_OFFSET) & UNCACHE_MASK)
#define __va(x)		((void *)((unsigned long) (x) - PHYS_OFFSET + PAGE_OFFSET))
#define __pa_symbol(x)  __pa(RELOC_HIDE((unsigned long)(x), 0))

#define MAP_NR(addr)       (((unsigned long)(addr)-PAGE_OFFSET-CONFIG_RAM_BASE) \
                               >> PAGE_SHIFT)

#define virt_to_page(kaddr)	(mem_map + ((__pa(kaddr)-CONFIG_RAM_BASE) \
                                    >> PAGE_SHIFT))

#define VALID_PAGE(page)	((page - mem_map) < max_mapnr)

#define VM_DATA_DEFAULT_FLAGS  (VM_READ | VM_WRITE | VM_EXEC | \
				VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

#define UNCACHE_ADDR(addr)	((addr) - PAGE_OFFSET + UNCACHE_BASE)
#define CACHE_ADDR(addr)		((addr) - UNCACHE_BASE + PAGE_OFFSET)

/*
 * main RAM and kernel working space are coincident at 0x80000000, but to make
 * life more interesting, there's also an uncached virtual shadow at 0xb0000000
 * - these mappings are fixed in the MMU
 */

#define pfn_to_kaddr(pfn)       __va((pfn) << PAGE_SHIFT)

#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>

#endif /* __ASM_CSKY_PAGE_H */
