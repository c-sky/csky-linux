// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
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
	unsigned long attrs
	)
{
	unsigned long ret;
	void * vaddr;

	if (DMA_ATTR_NON_CONSISTENT & attrs)
		panic("csky %s panic DMA_ATTR_NON_CONSISTENT.\n", __func__);

	ret =  __get_free_pages((gfp | __GFP_NORETRY) & (~__GFP_HIGHMEM), get_order(size));
	if (!ret) {
		pr_err("csky %s no more free pages, %ld.\n", __func__, ret);
		return NULL;
	}

	memset((void *)ret, 0, size);

	dma_wbinv_range(ret, ret + size);

	*dma_handle = virt_to_phys((void*)ret);

	vaddr = (void *) UNCACHE_ADDR(ret);

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

	free_pages(addr, get_order(size));
}

static inline void __dma_sync(
	unsigned long addr,
	size_t size,
	enum dma_data_direction direction)
{
	switch (direction) {
	case DMA_TO_DEVICE:
		dma_wb_range(addr, addr+size);
		break;
	case DMA_FROM_DEVICE:
	case DMA_BIDIRECTIONAL:
		dma_wbinv_range(addr, addr+size);
		break;
	default:
		BUG();
	}
}

static int csky_dma_map_sg(
	struct device *dev,
	struct scatterlist *sglist,
	int nelems,
	enum dma_data_direction dir,
	unsigned long attrs
	)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sglist, sg, nelems, i)
		sg->dma_address = dma_map_page_attrs(dev, sg_page(sg), sg->offset, sg->length, dir, attrs);

	return nelems;
}

static void csky_dma_unmap_sg(
	struct device *dev,
	struct scatterlist *sglist,
	int nelems,
	enum dma_data_direction dir,
	unsigned long attrs
	)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sglist, sg, nelems, i)
		dma_unmap_page_attrs(dev, sg_dma_address(sg), sg_dma_len(sg), dir, attrs);
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

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr, size, direction);
}

static void csky_dma_sync_single_for_all(
	struct device *dev,
	dma_addr_t handle,
	size_t size,
	enum dma_data_direction dir
	)
{
	unsigned int offset = handle & (PAGE_SIZE - 1);
	struct page *page = pfn_to_page(__phys_to_pfn(handle));
	unsigned long addr;

	addr = (unsigned long) page_address(page) + offset;

	__dma_sync(addr, size, dir);
}

static void csky_dma_sync_sg_for_cpu(
	struct device *dev,
	struct scatterlist *sglist,
	int nelems,
	enum dma_data_direction direction
	)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nelems, i)
		__dma_sync((unsigned long)page_address(sg_page(sg)),
				sg->length, direction);
}

static void csky_dma_sync_sg_for_device(
	struct device *dev,
	struct scatterlist *sglist,
	int nelems,
	enum dma_data_direction direction
	)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nelems, i)
		__dma_sync((unsigned long)page_address(sg_page(sg)),
				sg->length, direction);
}

int csky_dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

int csky_dma_supported(struct device *dev, u64 mask)
{
	return DMA_BIT_MASK(32);
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
	.sync_single_for_cpu	= csky_dma_sync_single_for_all,
	.sync_single_for_device	= csky_dma_sync_single_for_all,
	.sync_sg_for_cpu	= csky_dma_sync_sg_for_cpu,
	.sync_sg_for_device	= csky_dma_sync_sg_for_device,

	.mapping_error		= csky_dma_mapping_error,
	.dma_supported		= csky_dma_supported,
};
EXPORT_SYMBOL(csky_dma_map_ops);

