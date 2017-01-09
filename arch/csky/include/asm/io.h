#ifndef __ASM_CSKY_IO_H
#define __ASM_CSKY_IO_H

#include <hal/pgtable-bits.h>
#include <linux/types.h>

extern void __iomem *ioremap(phys_addr_t offset, size_t size);

extern inline void *ioremap_nocache(phys_addr_t physaddr, unsigned long size)
{
	return ioremap(physaddr, size);
}

extern void iounmap(void *addr);

extern int remap_area_pages(unsigned long address, phys_addr_t phys_addr,
		size_t size, unsigned long flags);

#define ioremap_wc ioremap_nocache
#define ioremap_wt ioremap_nocache

#include <asm-generic/io.h>

#endif /* __ASM_CSKY_IO_H */
