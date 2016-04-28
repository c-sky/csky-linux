#include <linux/types.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/io.h>
#include <linux/cache.h>
#include <asm/cache.h>

static void *csky_dma_alloc(
	struct device *dev,
	size_t size,
	dma_addr_t *dma_handle,
	gfp_t gfp,
	struct dma_attrs *attrs
	)
{
	unsigned int ret;

	/* fixme, why gfp? */
	gfp &= ~(__GFP_DMA | __GFP_HIGHMEM);
	gfp |= __GFP_ZERO;

	ret = (unsigned int) __get_free_pages(gfp, get_order(size));
	if (!ret)
		return NULL;

	memset((void *)ret, 0, size);
	*dma_handle = virt_to_phys((void*)ret);

	if (!dma_get_attr(DMA_ATTR_NON_CONSISTENT, attrs)) {
		cache_op_range(
			ret, ret+size,
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
		ret = UNCACHE_ADDR(ret);
	}

	return (void *)ret;
}

static void csky_dma_free(
	struct device *dev,
	size_t size,
	void *vaddr,
	dma_addr_t dma_handle,
	struct dma_attrs *attrs
	)
{
	unsigned long addr = (unsigned long) vaddr;
	int order = get_order(size);

	if (!dma_get_attr(DMA_ATTR_NON_CONSISTENT, attrs))
		addr = CACHE_ADDR(addr);

	free_pages(addr, order);
}

static inline void __dma_sync(
	unsigned long addr,
	size_t size,
	enum dma_data_direction direction)
{
	switch (direction) {
	case DMA_TO_DEVICE:
		cache_op_range(
			addr, addr+size,
			DATA_CACHE|
			CACHE_CLR);
		break;

	case DMA_FROM_DEVICE:
		cache_op_range(
			addr, addr+size,
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
		break;

	case DMA_BIDIRECTIONAL:
		cache_op_range(
			addr, addr+size,
			DATA_CACHE|
			CACHE_CLR|
			CACHE_INV);
		break;

	default:
		BUG();
	}
}

static int csky_dma_map_sg(
	struct device *dev,
	struct scatterlist *sg,
	int nents,
	enum dma_data_direction direction,
	struct dma_attrs *attrs
	)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nents; i++, sg++) {
		unsigned long addr;

		addr = (unsigned long) sg_virt(sg);
		if (addr)
			__dma_sync(addr, sg->length, direction);
		sg->dma_address = virt_to_phys((void *)addr);
	}

	return nents;
}

void csky_dma_unmap_sg(
	struct device *dev,
	struct scatterlist *sg,
	int nhwentries,
	enum dma_data_direction direction,
	struct dma_attrs *attrs
	)
{
	unsigned long addr;
	int i;

	BUG_ON(direction == DMA_NONE);

	for (i = 0; i < nhwentries; i++, sg++) {
		if (direction != DMA_TO_DEVICE) {
			addr = (unsigned long) sg_virt(sg);
			if (addr)
				__dma_sync(addr, sg->length, direction);
		}
	}
}

static dma_addr_t csky_dma_map_page(
	struct device *dev,
	struct page *page,
	unsigned long offset,
	size_t size,
	enum dma_data_direction direction,
	struct dma_attrs *attrs
	)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long) page_address(page) + offset;

	__dma_sync(addr, size, direction);

	return page_to_phys(page) + offset;
}

static void csky_dma_unmap_page(
	struct device *dev,
	dma_addr_t dma_handle,
	size_t size,
	enum dma_data_direction direction,
	struct dma_attrs *attrs
	)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr, size, direction);
}

static void csky_dma_sync_single_for_cpu(
	struct device *dev,
	dma_addr_t dma_handle,
	size_t size,
	enum dma_data_direction direction
	)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr, size, direction);
}

static void csky_dma_sync_single_for_device(
	struct device *dev,
	dma_addr_t dma_handle,
	size_t size,
	enum dma_data_direction direction
	)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr, size, direction);
}

static void csky_dma_sync_sg_for_cpu(
	struct device *dev,
	struct scatterlist *sg,
	int nelems,
	enum dma_data_direction direction
	)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++) {
		__dma_sync((unsigned long)page_address(sg_page(sg)),
				sg->length, direction);
	}
}

static void csky_dma_sync_sg_for_device(
	struct device *dev,
	struct scatterlist *sg,
	int nelems,
	enum dma_data_direction direction
	)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++) {
		__dma_sync((unsigned long)page_address(sg_page(sg)),
				sg->length, direction);
	}
}

struct dma_map_ops csky_dma_map_ops = {
	.alloc			= csky_dma_alloc,
	.free			= csky_dma_free,
	.mmap			= NULL,
	.get_sgtable		= NULL,
	.map_page		= csky_dma_map_page,
	.unmap_page		= csky_dma_unmap_page,

	.map_sg			= csky_dma_map_sg,
	.unmap_sg		= csky_dma_unmap_sg,
	.sync_single_for_cpu	= csky_dma_sync_single_for_cpu,
	.sync_single_for_device	= csky_dma_sync_single_for_device,
	.sync_sg_for_cpu	= csky_dma_sync_sg_for_cpu,
	.sync_sg_for_device	= csky_dma_sync_sg_for_device,

	.mapping_error		= NULL,
	.dma_supported		= NULL,
	.set_dma_mask		= NULL,
};
EXPORT_SYMBOL(csky_dma_map_ops);

