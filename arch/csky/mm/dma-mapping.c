#include <linux/types.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/io.h>
#include <linux/cache.h>
#include <asm/cache.h>

/*
 * Some SOC set the ram and io in a seperate addr, then uncached ram couldn't be
 * reached Directlly. We must use remap to fit it, but it will cause problem when
 * some dma_alloc happens in irq/soft-irq context. eg: LC-235 in jira.
 */
#define BUGFIX_LC235

static void *csky_dma_alloc(
	struct device *dev,
	size_t size,
	dma_addr_t *dma_handle,
	gfp_t gfp,
	unsigned long attrs
	)
{
	unsigned long ret;
	void * vaddr;
#ifndef BUGFIX_LC235
	struct page *page;
	pgprot_t prot;
#endif

	if (DMA_ATTR_NON_CONSISTENT & attrs)
		panic("csky %s panic DMA_ATTR_NON_CONSISTENT.\n", __func__);

	ret =  __get_free_pages(gfp, get_order(size));
	if (!ret) {
		pr_err("csky %s no more free pages, %ld.\n", __func__, ret);
		return NULL;
	}

	memset((void *)ret, 0, size);

	cache_op_range(ret, ret + size,
		DATA_CACHE|CACHE_CLR|CACHE_INV);

	*dma_handle = virt_to_phys((void*)ret);
#ifndef BUGFIX_LC235
	prot = __pgprot(_PAGE_PRESENT | __READABLE | __WRITEABLE |
			_PAGE_GLOBAL | _CACHE_UNCACHED);

	page = virt_to_page(ret);

	vaddr = dma_common_contiguous_remap(page, PAGE_ALIGN(size), VM_USERMAP,
			prot, __builtin_return_address(0));
	if (!vaddr) {
		BUG();
	}
#else
	vaddr = (void *) UNCACHE_ADDR(ret);
#endif

	return vaddr;
}

static void csky_dma_free(
	struct device *dev,
	size_t size,
	void *vaddr,
	dma_addr_t dma_handle,
	unsigned long attrs
	)
{
	unsigned long addr = (unsigned long)phys_to_virt(dma_handle);

#ifndef BUGFIX_LC235
	vunmap(vaddr);
#endif
	free_pages(addr, get_order(size));
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
	unsigned long attrs
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
	unsigned long attrs
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
	unsigned long attrs
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
	unsigned long attrs
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

int csky_dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

int csky_dma_supported(struct device *dev, u64 mask)
{
	return 1;
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

	.mapping_error		= csky_dma_mapping_error,
	.dma_supported		= csky_dma_supported,
	.set_dma_mask		= NULL,
};
EXPORT_SYMBOL(csky_dma_map_ops);

