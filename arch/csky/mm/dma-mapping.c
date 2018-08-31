// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/cache.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/genalloc.h>
#include <linux/highmem.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/types.h>
#include <linux/version.h>
#include <asm/cache.h>

static struct gen_pool *atomic_pool;
static size_t atomic_pool_size __initdata = SZ_256K;

static int __init early_coherent_pool(char *p)
{
	atomic_pool_size = memparse(p, &p);
	return 0;
}
early_param("coherent_pool", early_coherent_pool);

static int __init atomic_pool_init(void)
{
	struct page *page;
	size_t size = atomic_pool_size;
	void *ptr;
	int ret;

	atomic_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!atomic_pool)
		BUG();

	page = alloc_pages(GFP_KERNEL | GFP_DMA, get_order(size));
	if (!page)
		BUG();

	ptr = dma_common_contiguous_remap(page, size, VM_ALLOC,
					  pgprot_noncached(PAGE_KERNEL),
					  __builtin_return_address(0));
	if (!ptr)
		BUG();

	ret = gen_pool_add_virt(atomic_pool, (unsigned long)ptr,
				page_to_phys(page), atomic_pool_size, -1);
	if (ret)
		BUG();

	gen_pool_set_algo(atomic_pool, gen_pool_first_fit_order_align, NULL);

	pr_info("DMA: preallocated %zu KiB pool for atomic coherent allocations\n",
		atomic_pool_size / 1024);
	pr_info("DMA: vaddr: 0x%x phy: 0x%lx, \n", (unsigned int)ptr, page_to_phys(page));

	return 0;
}
postcore_initcall(atomic_pool_init);

static void *csky_dma_alloc_atomic(
	struct device *dev,
	size_t size,
	dma_addr_t *dma_handle
	)
{
	unsigned long addr;

	addr = gen_pool_alloc(atomic_pool, size);
	if (addr)
		*dma_handle = gen_pool_virt_to_phys(atomic_pool, addr);

	return (void *)addr;
}

static void csky_dma_free_atomic(
	struct device *dev,
	size_t size,
	void *vaddr,
	dma_addr_t dma_handle,
	unsigned long attrs
	)
{
	gen_pool_free(atomic_pool, (unsigned long)vaddr, size);
}

static void __dma_clear_buffer(struct page *page, size_t size)
{
	if (PageHighMem(page)) {
		unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;

		do {
			void *ptr = kmap_atomic(page);
			size_t _size = (size < PAGE_SIZE)? size: PAGE_SIZE;

			memset(ptr, 0, _size);
			dma_wbinv_range((unsigned long)ptr,(unsigned long)ptr + _size);

			kunmap_atomic(ptr);

			page++;
			size -= PAGE_SIZE;
			count--;
		} while (count);
	} else {
		void *ptr = page_address(page);
		memset(ptr, 0, size);
		dma_wbinv_range((unsigned long)ptr,(unsigned long)ptr + size);
	}
}

static void *csky_dma_alloc_nonatomic(
	struct device *dev,
	size_t size,
	dma_addr_t *dma_handle,
	gfp_t gfp,
	unsigned long attrs
	)
{
	void  *vaddr;
	struct page *page;
	unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;

	if (DMA_ATTR_NON_CONSISTENT & attrs)
		BUG();

	if (IS_ENABLED(CONFIG_DMA_CMA))
#if (LINUX_VERSION_CODE >> 8) == (KERNEL_VERSION(4,9,0) >> 8)
		page = dma_alloc_from_contiguous(dev, count, get_order(size));
#endif
#if (LINUX_VERSION_CODE >> 8) != (KERNEL_VERSION(4,9,0) >> 8)
		page = dma_alloc_from_contiguous(dev, count, get_order(size), gfp);
#endif
	else
		page = alloc_pages(gfp, get_order(size));

	if (!page) {
		pr_err("csky %s no more free pages.\n", __func__);
		return NULL;
	}

	*dma_handle = page_to_phys(page);

	__dma_clear_buffer(page, size);

	if (attrs & DMA_ATTR_NO_KERNEL_MAPPING)
		return page;

	vaddr = dma_common_contiguous_remap(page, PAGE_ALIGN(size), VM_USERMAP,
				pgprot_noncached(PAGE_KERNEL) , __builtin_return_address(0));
	if (!vaddr)
		BUG();

	return vaddr;
}

static void csky_dma_free_nonatomic(
	struct device *dev,
	size_t size,
	void *vaddr,
	dma_addr_t dma_handle,
	unsigned long attrs
	)
{
	struct page *page = phys_to_page(dma_handle);
	unsigned int count = PAGE_ALIGN(size) >> PAGE_SHIFT;

	if ((unsigned int)vaddr >= VMALLOC_START)
		dma_common_free_remap(vaddr, size, VM_USERMAP);

	if (IS_ENABLED(CONFIG_DMA_CMA))
		dma_release_from_contiguous(dev, page, count);
	else
		__free_pages(page, get_order(size));
}

static void *csky_dma_alloc(
	struct device *dev,
	size_t size,
	dma_addr_t *dma_handle,
	gfp_t gfp,
	unsigned long attrs
	)
{
	if (gfpflags_allow_blocking(gfp))
		return csky_dma_alloc_nonatomic(dev, size, dma_handle, gfp, attrs);
	else
		return csky_dma_alloc_atomic(dev, size, dma_handle);
}

static void csky_dma_free(
	struct device *dev,
	size_t size,
	void *vaddr,
	dma_addr_t dma_handle,
	unsigned long attrs
	)
{
	if (!addr_in_gen_pool(atomic_pool, (unsigned int) vaddr, size))
		csky_dma_free_nonatomic(dev, size, vaddr, dma_handle, attrs);
	else
		csky_dma_free_atomic(dev, size, vaddr, dma_handle, attrs);
}

static void __dma_sync(
	void *va,
	struct page *page,
	unsigned long offset,
	size_t size,
	enum dma_data_direction direction)
{
	unsigned long vaddr = (unsigned long) va;

	if (va == NULL && PageHighMem(page))
		vaddr = (unsigned long) kmap_atomic(page);

	switch (direction) {
	case DMA_TO_DEVICE:
		dma_wb_range(vaddr + offset, vaddr + offset + size);
		break;
	case DMA_FROM_DEVICE:
	case DMA_BIDIRECTIONAL:
		dma_wbinv_range(vaddr + offset, vaddr + offset + size);
		break;
	default:
		BUG();
	}

	if (va == NULL && PageHighMem(page))
		kunmap_atomic((void *) vaddr);
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
	__dma_sync(page_address(page), page, offset, size, direction);

	return page_to_phys(page) + offset;
}

static void csky_dma_unmap_page(
	struct device *dev,
	dma_addr_t handle,
	size_t size,
	enum dma_data_direction direction,
	unsigned long attrs
	)
{
	unsigned long offset = handle & ~PAGE_MASK;
	struct page *page = pfn_to_page(__phys_to_pfn(handle));

	__dma_sync(page_address(page), page, offset, size, direction);
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
		sg->dma_address = csky_dma_map_page(dev, sg_page(sg), sg->offset, sg->length, dir, attrs);

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
		csky_dma_unmap_page(dev, sg_dma_address(sg), sg_dma_len(sg), dir, attrs);
}

static void csky_dma_sync_single_for_all(
	struct device *dev,
	dma_addr_t handle,
	size_t size,
	enum dma_data_direction dir
	)
{
	unsigned int offset = handle & ~PAGE_MASK;
	struct page *page = pfn_to_page(__phys_to_pfn(handle));

	__dma_sync(page_address(page), page, offset, size, dir);
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
		__dma_sync(page_address(sg_page(sg)), sg_page(sg), 0,
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
		__dma_sync(page_address(sg_page(sg)), sg_page(sg), 0,
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

