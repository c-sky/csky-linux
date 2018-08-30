// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ASM_DMA_MAPPING_H
#define __ASM_DMA_MAPPING_H

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

#endif /* __ASM_DMA_MAPPING_H */
