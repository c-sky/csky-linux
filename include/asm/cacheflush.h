#ifndef __ASM_CSKY_CACHEFLUSH_H
#define __ASM_CSKY_CACHEFLUSH_H

#include <linux/compiler.h>
#include <asm/string.h>
#include <asm/cache.h>

#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1

/*
 * as the CK610 cache is PIVT and is 16K, we will flush all
 * if the flush range is larger than a page(4K).
 *
 * it is hard to decide how large the scale should be flush all,
 * so, this conclusion(large than 4K) is reference to mips and arm.
 */

#define flush_cache_mm(mm)		cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV)
#define flush_cache_page(vma,page,pfn)	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV)
#define flush_cache_dup_mm(mm)		cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV)
#define flush_icache_page(vma, page)	cache_op_all(INS_CACHE|CACHE_INV)
#define flush_dcache_page(page)		cache_op_all(DATA_CACHE|CACHE_CLR|CACHE_INV)

#define flush_cache_range(mm,start,end)	cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV)
#define flush_cache_vmap(start, end)	cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV)
#define flush_cache_vunmap(start, end)  cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV)
#define flush_icache_range(start, end)	cache_op_range(start, end, INS_CACHE|CACHE_INV)

#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
do{ \
	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV); \
	memcpy(dst, src, len); \
	cache_op_all(INS_CACHE|CACHE_INV); \
}while(0)

#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
do{ \
	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV); \
	memcpy(dst, src, len); \
}while(0)

#define flush_dcache_mmap_lock(mapping)		do{}while(0)
#define flush_dcache_mmap_unlock(mapping)	do{}while(0)

#endif /* __ASM_CSKY_CACHEFLUSH_H */

