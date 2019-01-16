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

extern void arch_dma_prep_coherent(struct page *page, size_t size);
void arch_sync_dma_for_cpu(struct device *dev, phys_addr_t paddr,
			size_t size, enum dma_data_direction dir);
void arch_sync_dma_for_device(struct device *dev, phys_addr_t paddr,
			size_t size, enum dma_data_direction dir);
int __init dma_atomic_pool_init(gfp_t gfp, pgprot_t prot)
{
	struct page *page;
	void *ptr;
	int ret;

	atomic_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!atomic_pool)
		BUG();

	page = alloc_pages(gfp, get_order(atomic_pool_size));
	if (!page)
		BUG();

	ptr = dma_common_contiguous_remap(page, atomic_pool_size, VM_ALLOC,
					  prot, __builtin_return_address(0));
	if (!ptr)
		BUG();

	arch_dma_prep_coherent(page, atomic_pool_size);

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

	arch_dma_prep_coherent(page, size);

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

static void csky_dma_sync_single_for_all(
	struct device *dev,
	dma_addr_t addr,
	size_t size,
	enum dma_data_direction dir
	)
{
	phys_addr_t paddr = dma_to_phys(dev, addr);

	arch_sync_dma_for_device(dev, paddr, size, dir);
}

static dma_addr_t csky_dma_map_page(
	struct device *dev,
	struct page *page,
	unsigned long offset,
	size_t size,
	enum dma_data_direction dir,
	unsigned long attrs
	)
{
	phys_addr_t phys = page_to_phys(page) + offset;
	dma_addr_t dma_addr = phys_to_dma(dev, phys);

	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		arch_sync_dma_for_device(dev, phys, size, dir);

	return dma_addr;
}

static void csky_dma_unmap_page(
	struct device *dev,
	dma_addr_t addr,
	size_t size,
	enum dma_data_direction dir,
	unsigned long attrs
	)
{
	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		csky_dma_sync_single_for_all(dev, addr, size, dir);
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

static void csky_dma_sync_sg_for_cpu(
	struct device *dev,
	struct scatterlist *sglist,
	int nelems,
	enum dma_data_direction dir
	)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nelems, i)
		arch_sync_dma_for_cpu(dev, sg_phys(sg), sg->length, dir);
}

static void csky_dma_sync_sg_for_device(
	struct device *dev,
	struct scatterlist *sglist,
	int nelems,
	enum dma_data_direction dir
	)
{
	int i;
	struct scatterlist *sg;

	for_each_sg(sglist, sg, nelems, i)
		arch_sync_dma_for_device(dev, sg_phys(sg), sg->length, dir);
}

int csky_dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

int csky_dma_supported(struct device *dev, u64 mask)
{
	return DMA_BIT_MASK(32);
}

static int csky_dma_mmap(struct device *dev, struct vm_area_struct *vma,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
	int ret = -ENXIO;
	unsigned long user_count = vma_pages(vma);
	unsigned long count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	unsigned long pfn = dma_to_phys(dev, dma_addr) >> PAGE_SHIFT;
	unsigned long off = vma->vm_pgoff;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

#if (LINUX_VERSION_CODE >> 8) == (KERNEL_VERSION(4,9,0) >> 8)
	if (dma_mmap_from_coherent(dev, vma, cpu_addr, size, &ret))
#else
	if (dma_mmap_from_dev_coherent(dev, vma, cpu_addr, size, &ret))
#endif
		return ret;

	if (off < count && user_count <= (count - off)) {
		ret = remap_pfn_range(vma, vma->vm_start,
				      pfn + off,
				      user_count << PAGE_SHIFT,
				      vma->vm_page_prot);
	}

	return ret;
}

struct dma_map_ops csky_dma_map_ops = {
	.alloc			= csky_dma_alloc,
	.free			= csky_dma_free,
	.mmap			= csky_dma_mmap,
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
