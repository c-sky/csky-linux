#ifndef __ASM_DMA_MAPPING_H
#define __ASM_DMA_MAPPING_H

extern struct dma_map_ops csky_dma_map_ops;

static inline struct dma_map_ops *get_dma_ops(struct device *dev)
{
	return &csky_dma_map_ops;
}

#endif /* __ASM_DMA_MAPPING_H */
