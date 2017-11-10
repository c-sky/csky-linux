#ifndef __ASM_CSKY_CACHEFLUSH_H
#define __ASM_CSKY_CACHEFLUSH_H

#include <linux/compiler.h>
#include <asm/string.h>
#include <asm/cache.h>

#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
extern void flush_dcache_page(struct page *);

#define flush_cache_mm(mm)		cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0)
#define flush_cache_page(vma,page,pfn)	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0)
#define flush_cache_dup_mm(mm)		cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0)
#define flush_icache_page(vma, page)	cache_op_all(INS_CACHE|CACHE_INV, 0)

#define flush_cache_range(mm,start,end)	cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0)
#define flush_cache_vmap(start, end)	cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0)
#define flush_cache_vunmap(start, end)  cache_op_range(start, end, INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0)
#define flush_icache_range(start, end)	cache_op_range(start, end, INS_CACHE|CACHE_INV, 0)

#define copy_from_user_page(vma, page, vaddr, dst, src, len) \
do{ \
	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0); \
	memcpy(dst, src, len); \
	cache_op_all(INS_CACHE|CACHE_INV, 0); \
}while(0)

#define copy_to_user_page(vma, page, vaddr, dst, src, len) \
do{ \
	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 0); \
	memcpy(dst, src, len); \
}while(0)

#define flush_dcache_mmap_lock(mapping)		do{}while(0)
#define flush_dcache_mmap_unlock(mapping)	do{}while(0)

#endif /* __ASM_CSKY_CACHEFLUSH_H */

