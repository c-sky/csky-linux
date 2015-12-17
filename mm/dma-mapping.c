/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 * Copyright (C) 2010  Hu Junshan (junshan_hu@c-sky.com)
 */

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include <asm/cache.h>
#include <asm/cacheflush.h>
#include <asm/io.h>

#ifndef CONFIG_MMU
static spinlock_t dma_page_lock;
static char *dma_page;
static unsigned int dma_pages;
static unsigned long dma_base;
static unsigned long dma_size;
static unsigned int dma_initialized;

void dma_alloc_init(unsigned long start, unsigned long end)
{
	spin_lock_init(&dma_page_lock);
	dma_initialized = 0;
	
	dma_page = (char *)__get_free_page(GFP_KERNEL);
	memset(dma_page, 0, PAGE_SIZE);
	dma_base = PAGE_ALIGN(start);
	dma_size = PAGE_ALIGN(end) - PAGE_ALIGN(start);
	dma_pages = dma_size >> PAGE_SHIFT;
	memset((void *)dma_base, 0, DMA_UNCACHED_REGION);
	dma_initialized = 1;
	
	printk(KERN_INFO "%s: dma_page @ 0x%p - %d pages at 0x%08lx\n", __func__,
	       dma_page, dma_pages, dma_base);
}

static inline unsigned int get_pages(size_t size)
{
	return ((size - 1) >> PAGE_SHIFT) + 1;
}

static unsigned long __alloc_dma_pages(unsigned int pages)
{
	unsigned long ret = 0, flags;
	int i, count = 0;
	
	if (dma_initialized == 0)
		dma_alloc_init(CK_UNCACHED_RAM_BASE, 
		        CK_UNCACHED_RAM_BASE + DMA_UNCACHED_REGION);
	
	spin_lock_irqsave(&dma_page_lock, flags);
	
	for (i = 0; i < dma_pages;) {
		if (dma_page[i++] == 0) {
			if (++count == pages) {
				while (count--)
				        dma_page[--i] = 1;
				ret = dma_base + (i << PAGE_SHIFT);
				break;
			}
		} else
		        count = 0;
	}
	spin_unlock_irqrestore(&dma_page_lock, flags);
	return ret;
}

static void __free_dma_pages(unsigned long addr, unsigned int pages)
{
	unsigned long page = (addr - dma_base) >> PAGE_SHIFT;
	unsigned long flags;
	int i;
	
	if ((page + pages) > dma_pages) {
		printk(KERN_ERR "%s: freeing outside range.\n", __func__);
		BUG();
	}
	
	spin_lock_irqsave(&dma_page_lock, flags);
	for (i = page; i < page + pages; i++) {
		dma_page[i] = 0;
	}
	spin_unlock_irqrestore(&dma_page_lock, flags);
}

#endif /*CONFIG_MMU*/

static gfp_t massage_gfp_flags(const struct device *dev, gfp_t gfp)
{
	/* ignore region specifiers */
	gfp &= ~(__GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM);

#ifdef CONFIG_ZONE_DMA
	if (dev == NULL)
		gfp |= __GFP_DMA;
	else if (dev->coherent_dma_mask < DMA_BIT_MASK(24))
		gfp |= __GFP_DMA;
	else
#endif
#ifdef CONFIG_ZONE_DMA32
	     if (dev->coherent_dma_mask < DMA_BIT_MASK(32))
		gfp |= __GFP_DMA32;
	else
#endif
		;

	/* Don't invoke OOM killer */
	gfp |= __GFP_NORETRY;

	return gfp;
}

#ifdef CONFIG_PHYSICAL_BASE_CHANGE
void *dma_alloc_coherent(struct device *dev, size_t size,
        dma_addr_t * dma_handle, gfp_t gfp)
{
	void *ret;
	struct vm_struct * area;
	void *addr;
	if (dma_alloc_from_coherent(dev, size, dma_handle, &ret))
		return ret;

	gfp = massage_gfp_flags(dev, gfp);

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if(ret)
	{
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
		dma_cache_wback_inv((unsigned long) ret, size);
		area = get_vm_area(size, VM_IOREMAP);
		if (!area)
			return NULL;
		addr = area->addr;
		if (remap_area_pages((unsigned long)addr, (phys_t)(*dma_handle), size, _CACHE_UNCACHED)) {
			vunmap(addr);
			return NULL;
		}
	}

	return (void *)addr;
}

EXPORT_SYMBOL(dma_alloc_coherent);

void dma_free_coherent(struct device *dev, size_t size, void *vaddr,
        dma_addr_t dma_handle)
{
	unsigned long addr = (unsigned long)phys_to_virt(dma_handle);
	int order = get_order(size);

	if (dma_release_from_coherent(dev, order, addr))
		return;

	vfree((void *) (PAGE_MASK & (unsigned long) vaddr));

	free_pages(addr, order);
}

EXPORT_SYMBOL(dma_free_coherent);

#else /* PHYSICAL_BASE_CHANGE */

void *dma_alloc_coherent(struct device *dev, size_t size,
        dma_addr_t * dma_handle, gfp_t gfp)
{
	void *ret;	
#if defined(CONFIG_MMU)	
	if (dma_alloc_from_coherent(dev, size, dma_handle, &ret))
		return ret;

	gfp = massage_gfp_flags(dev, gfp);

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if(ret)
	{
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
		dma_cache_wback_inv((unsigned long) ret, size);
		ret = UNCACHE_ADDR(ret);
	}
#else
	ret = (void *)__alloc_dma_pages(get_pages(size));

	if (ret) {
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
	}
#endif
	return (void *)ret;
}

EXPORT_SYMBOL(dma_alloc_coherent);

void dma_free_coherent(struct device *dev, size_t size, void *vaddr,
        dma_addr_t dma_handle)
{
#if defined(CONFIG_MMU)
	unsigned long addr = (unsigned long) vaddr;
	int order = get_order(size);

	if (dma_release_from_coherent(dev, order, vaddr))
		return;
	
	addr = CACHE_ADDR(addr);

	free_pages(addr, order);
#else
	__free_dma_pages((unsigned long)vaddr, get_pages(size));
#endif
}

EXPORT_SYMBOL(dma_free_coherent);

#endif /* !PHYSICAL_BASE_CHANGE */

void dma_free_noncoherent(struct device *dev, size_t size,
                         void *vaddr, dma_addr_t dma_handle)
{
	free_pages((unsigned long) vaddr, get_order(size));
}
EXPORT_SYMBOL(dma_free_noncoherent);

void *dma_alloc_noncoherent(struct device *dev, size_t size,
                           dma_addr_t *dma_handle, gfp_t gfp)
{
	void *ret;

	gfp = massage_gfp_flags(dev, gfp);

	ret = (void *) __get_free_pages(gfp, get_order(size));

	if (ret != NULL) {
		memset(ret, 0, size);
		*dma_handle = virt_to_phys(ret);
	}

	return ret;
}

EXPORT_SYMBOL(dma_alloc_noncoherent);

static inline void __dma_sync(unsigned long addr, size_t size,
	enum dma_data_direction direction)
{
	switch (direction) {
	case DMA_TO_DEVICE:
		dma_cache_wback(addr, size);
		break;

	case DMA_FROM_DEVICE:
		dma_cache_inv(addr, size);
		break;

	case DMA_BIDIRECTIONAL:
		dma_cache_wback_inv(addr, size);
		break;

	default:
		BUG();
	}
}

void dma_cache_maint(const void *start, size_t size, int direction)
{
	__dma_sync( (unsigned long)start, size, (enum dma_data_direction)direction);
}

dma_addr_t dma_map_single(struct device *dev, void *ptr, size_t size,
    enum dma_data_direction direction)
{
	unsigned long addr = (unsigned long) ptr;
	
	__dma_sync(addr, size, direction);

	return virt_to_phys(ptr);
}

EXPORT_SYMBOL(dma_map_single);

void dma_unmap_single(struct device *dev, dma_addr_t dma_addr, size_t size,
    enum dma_data_direction direction)
{
	__dma_sync((unsigned long)phys_to_virt((unsigned long)dma_addr), size,
		           direction);
}

EXPORT_SYMBOL(dma_unmap_single);

int dma_map_sg(struct device *dev, struct scatterlist *sg, int nents,
	enum dma_data_direction direction)
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

EXPORT_SYMBOL(dma_map_sg);

dma_addr_t dma_map_page(struct device *dev, struct page *page,
    unsigned long offset, size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long) page_address(page) + offset;
	__dma_sync(addr, size, direction);

	return page_to_phys(page) + offset;
}

EXPORT_SYMBOL(dma_map_page);

void dma_unmap_sg(struct device *dev, struct scatterlist *sg, int nhwentries,
	enum dma_data_direction direction)
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

EXPORT_SYMBOL(dma_unmap_sg);

void dma_sync_single_for_cpu(struct device *dev, dma_addr_t dma_handle,
	size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_for_cpu);

void dma_sync_single_for_device(struct device *dev, dma_addr_t dma_handle,
	size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_for_device);

void dma_sync_single_range_for_cpu(struct device *dev, dma_addr_t dma_handle,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle);
	__dma_sync(addr + offset, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_range_for_cpu);

void dma_sync_single_range_for_device(struct device *dev, dma_addr_t dma_handle,
	unsigned long offset, size_t size, enum dma_data_direction direction)
{
	unsigned long addr;

	BUG_ON(direction == DMA_NONE);

	addr = (unsigned long)phys_to_virt((unsigned long)dma_handle); 
	__dma_sync(addr + offset, size, direction);
}

EXPORT_SYMBOL(dma_sync_single_range_for_device);

void dma_sync_sg_for_cpu(struct device *dev, struct scatterlist *sg, int nelems,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++) {
		__dma_sync((unsigned long)page_address(sg_page(sg)),
		           sg->length, direction);
	}
}

EXPORT_SYMBOL(dma_sync_sg_for_cpu);

void dma_sync_sg_for_device(struct device *dev, struct scatterlist *sg, int nelems,
	enum dma_data_direction direction)
{
	int i;

	BUG_ON(direction == DMA_NONE);

	/* Make sure that gcc doesn't leave the empty loop body.  */
	for (i = 0; i < nelems; i++, sg++) {
		__dma_sync((unsigned long)page_address(sg_page(sg)),
		           sg->length, direction);
	}
}

EXPORT_SYMBOL(dma_sync_sg_for_device);

int dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

EXPORT_SYMBOL(dma_mapping_error);

int dma_supported(struct device *dev, u64 mask)
{
	/*
	 * we fall back to GFP_DMA when the mask isn't all 1s,
	 * so we can't guarantee allocations that must be
	 * within a tighter range than GFP_DMA..
	 */
	if (mask < 0x00ffffff)
		return 0;

	return 1;
}

EXPORT_SYMBOL(dma_supported);

int dma_is_consistent(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

EXPORT_SYMBOL(dma_is_consistent);

void dma_cache_sync(struct device *dev, void *vaddr, size_t size,
	       enum dma_data_direction direction)
{
	BUG_ON(direction == DMA_NONE);

	__dma_sync((unsigned long)vaddr, size, direction);
}

EXPORT_SYMBOL(dma_cache_sync);


