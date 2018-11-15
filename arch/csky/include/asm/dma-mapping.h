// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ASM_DMA_MAPPING_H
#define __ASM_DMA_MAPPING_H

#include <linux/types.h>

extern struct dma_map_ops csky_dma_map_ops;

#if (LINUX_VERSION_CODE >> 8) == (KERNEL_VERSION(4,9,0) >> 8)
static inline struct dma_map_ops *get_dma_ops(struct device *dev)
#endif
#if (LINUX_VERSION_CODE >> 8) != (KERNEL_VERSION(4,9,0) >> 8)
static inline struct dma_map_ops *get_arch_dma_ops(struct bus_type *bus)
#endif
{
	return &csky_dma_map_ops;
}

static inline dma_addr_t phys_to_dma(struct device *dev, phys_addr_t paddr)
{
	dma_addr_t dev_addr = (dma_addr_t)paddr;

	return dev_addr - ((dma_addr_t)dev->dma_pfn_offset << PAGE_SHIFT);
}

static inline phys_addr_t dma_to_phys(struct device *dev, dma_addr_t dev_addr)
{
	phys_addr_t paddr = (phys_addr_t)dev_addr;

	return paddr + ((phys_addr_t)dev->dma_pfn_offset << PAGE_SHIFT);
}

#endif /* __ASM_DMA_MAPPING_H */
