/*
 * linux/arch/csky/mm/ioremap.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 C-SKY Microsystems co.,ltd.  All rights reserved.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 */
#include <linux/module.h>
#include <asm/addrspace.h>
#include <asm/byteorder.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>

static inline void remap_area_pte(pte_t * pte, unsigned long address,
	phys_addr_t size, phys_addr_t phys_addr, unsigned long flags)
{
	phys_addr_t end;
	unsigned long pfn;
	pgprot_t pgprot = __pgprot(_PAGE_GLOBAL | _PAGE_PRESENT | __READABLE
	                           | __WRITEABLE | flags);

	address &= ~PMD_MASK;
	end = address + size;
	if (end > PMD_SIZE)
		end = PMD_SIZE;
	if (address >= end)
		BUG();
	pfn = phys_addr >> PAGE_SHIFT;
	do {
		if (!pte_none(*pte)) {
			printk("remap_area_pte: page already exists\n");
			BUG();
		}
		set_pte(pte, pfn_pte(pfn, pgprot));
		address += PAGE_SIZE;
		phys_addr += PAGE_SIZE;
		pfn++;
		pte++;
	} while (address && (address < end));
}

static inline int remap_area_pmd(pmd_t * pmd, unsigned long address,
	phys_addr_t size, phys_addr_t phys_addr, unsigned long flags)
{
	phys_addr_t end;

	address &= ~PGDIR_MASK;
	end = address + size;
	if (end > PGDIR_SIZE)
		end = PGDIR_SIZE;
	phys_addr -= address;
	if (address >= end)
		BUG();
	do {
		pte_t * pte = pte_alloc_kernel(pmd, address);
		if (!pte)
			return -ENOMEM;
		remap_area_pte(pte, address, end - address, address + phys_addr, flags);
		address = (address + PMD_SIZE) & PMD_MASK;
		pmd++;
	} while (address && (address < end));
	return 0;
}

int remap_area_pages(unsigned long address, phys_addr_t phys_addr,
	phys_addr_t size, unsigned long flags)
{
	int error;
	pgd_t * dir;
	unsigned long end = address + size;

	phys_addr -= address;
	dir = pgd_offset(&init_mm, address);
	if (address >= end)
		BUG();
	do {
		pud_t *pud;
		pmd_t *pmd;

		error = -ENOMEM;
		pud = pud_alloc(&init_mm, dir, address);
                if (!pud)
                        break;
		pmd = pmd_alloc(&init_mm, pud, address);
		if (!pmd)
			break;
		if (remap_area_pmd(pmd, address, end - address,
					 phys_addr + address, flags))
			break;
		error = 0;
		address = (address + PGDIR_SIZE) & PGDIR_MASK;
		dir++;
	} while (address && (address < end));
	flush_tlb_all();
	return error;
}

EXPORT_SYMBOL(remap_area_pages);

/*
 * Allow physical addresses to be fixed up to help 36 bit
 * peripherals.
 */
static phys_addr_t def_fixup_bigphys_addr(phys_addr_t phys_addr, phys_addr_t size)
{
	return phys_addr;
}

phys_addr_t (*fixup_bigphys_addr)(phys_addr_t phys_addr, phys_addr_t size) =
                   def_fixup_bigphys_addr;

/*
 * Generic mapping function (not visible outside):
 */

/*
 * Remap an arbitrary physical address space into the kernel virtual
 * address space. Needed when the kernel wants to access high addresses
 * directly.
 *
 * NOTE! We need to allow non-page-aligned mappings too: we will obviously
 * have to convert them into an offset in a page-aligned mapping, but the
 * caller shouldn't need to know that small detail.
 */

#define IS_LOW512(addr) (!((phys_addr_t)(addr) & (phys_addr_t) ~0x1fffffffULL))

void __iomem * __ioremap(phys_addr_t phys_addr, phys_addr_t size, unsigned long flags)
{
	struct vm_struct * area;
	unsigned long offset;
	phys_addr_t last_addr;
	void * addr;

	phys_addr = fixup_bigphys_addr(phys_addr, size);

	/* Don't allow wraparound or zero size */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	/*
	 * Map uncached objects in the low 512mb of address space using KSEG1,
	 * otherwise map using page tables.
	 */
	if (IS_LOW512(phys_addr) && IS_LOW512(last_addr) &&
	    flags == _CACHE_UNCACHED)
		return (void __iomem *) KSEG1ADDR(phys_addr);

	/*
	 * Don't allow anybody to remap normal RAM that we're using..
	 */
	if (phys_addr < virt_to_phys(high_memory)) {
		char *t_addr, *t_end;
		struct page *page;

		t_addr = __va(phys_addr);
		t_end = t_addr + (size - 1);

		for(page = virt_to_page(t_addr); page <= virt_to_page(t_end); page++)
			if(!PageReserved(page))
				return NULL;
	}

	/*
	 * Mappings have to be page-aligned
	 */
	offset = phys_addr & ~PAGE_MASK;
	phys_addr &= PAGE_MASK;
	size = PAGE_ALIGN(last_addr + 1) - phys_addr;

	/*
	 * Ok, go for it..
	 */
	area = get_vm_area(size, VM_IOREMAP);
	if (!area)
		return NULL;
	addr = area->addr;
	if (remap_area_pages((unsigned long)addr, phys_addr, size, flags)) {
		 vunmap(addr);
		return NULL;
	}
	return (void __iomem *) (offset + (char *)addr);
}

void __iomem * __ioremap_mode(phys_addr_t offset, unsigned long size, unsigned long flags)
{
        if (__builtin_constant_p(offset) &&
            __builtin_constant_p(size) && __builtin_constant_p(flags)) {
                phys_addr_t phys_addr, last_addr;

                phys_addr = fixup_bigphys_addr(offset, size);

                /* Don't allow wraparound or zero size. */
                last_addr = phys_addr + size - 1;
                if (!size || last_addr < phys_addr)
                        return NULL;

                /*
                 * Map uncached objects in the low 512MB of address
                 * space using KSEG1.
                 */
                if (IS_LOW512(phys_addr) && IS_LOW512(last_addr) &&
                    flags == _CACHE_UNCACHED)
                        return (void __iomem *)
                                (unsigned long)KSEG1ADDR(phys_addr);
        }

        return __ioremap(offset, size, flags);
}
EXPORT_SYMBOL(__ioremap_mode);

#define IS_KSEG1(addr) (((unsigned long)(addr) & ~0x1fffffffUL) == KSEG1)

void iounmap(void *addr)
{
	if (!IS_KSEG1(addr))
		return vfree((void *) (PAGE_MASK & (unsigned long) addr));
}

EXPORT_SYMBOL(__ioremap);
EXPORT_SYMBOL(iounmap);
