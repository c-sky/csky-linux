#ifndef __ASM_CSKY_IO_H
#define __ASM_CSKY_IO_H

#include <abi/pgtable-bits.h>
#include <linux/types.h>
#include <linux/version.h>

extern void __iomem *ioremap(phys_addr_t offset, size_t size);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
extern inline void *ioremap_nocache(phys_addr_t physaddr, unsigned long size)
{
	return ioremap(physaddr, size);
}
#endif

extern void iounmap(void *addr);

extern int remap_area_pages(unsigned long address, phys_addr_t phys_addr,
		size_t size, unsigned long flags);

#define ioremap_wc ioremap_nocache
#define ioremap_wt ioremap_nocache

#include <asm-generic/io.h>

#endif /* __ASM_CSKY_IO_H */
