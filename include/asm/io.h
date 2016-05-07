#ifndef __ASM_CSKY_IO_H
#define __ASM_CSKY_IO_H

#include <hal/pgtable-bits.h>
#include <asm/types.h>

extern void __iomem * __ioremap_mode(phys_addr_t offset, unsigned long size,
	unsigned long flags);

extern inline void *ioremap(phys_addr_t physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, _CACHE_UNCACHED);
}

extern inline void *ioremap_nocache(phys_addr_t physaddr, unsigned long size)
{
	return __ioremap_mode(physaddr, size, _CACHE_UNCACHED);
}

extern void iounmap(void *addr);

#define ioremap_wc ioremap_nocache
#define ioremap_wt ioremap_nocache

#include <asm-generic/io.h>

#endif /* __ASM_CSKY_IO_H */
