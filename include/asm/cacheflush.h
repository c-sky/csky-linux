#ifndef __ASM_CSKY_CACHEFLUSH_H
#define __ASM_CSKY_CACHEFLUSH_H

#include <linux/compiler.h>
#include <asm/string.h>
#include <asm/outercache.h>

/*
 * Cache flushing:
 *  - flush_cache_all() flushes entire cache
 *  - flush_cache_mm(mm) flushes the specified mm context's cache lines
 *  - flush_cache_page(mm, vmaddr) flushes a single page
 *  - flush_cache_range(mm, start, end) flushes a range of pages
 *  - flush_page_to_ram(page) write back kernel page to ram
 *  - flush_icache_range(start, end) flush a range of instructions
 *
 * CK640 specific flush operations:
 *
 *  - flush_cache_sigtramp() flush signal trampoline
 *  - flush_icache_all() flush the entire instruction cache
 */
struct mm_struct;
struct vm_area_struct;
struct page;

extern void _flush_cache_all(void);
extern void _flush_cache_mm(struct mm_struct *mm);
extern void _flush_cache_range(struct vm_area_struct *mm,
                           unsigned long start, unsigned long end);
extern void _flush_cache_page(struct vm_area_struct *vma, unsigned long page);
extern void _flush_dcache_page(struct page * page);
extern void _flush_icache_range(unsigned long start, unsigned long end);
extern void _flush_icache_page(struct vm_area_struct *vma, struct page *page);

extern void _flush_cache_sigtramp(unsigned long addr);
extern void _flush_icache_all(void);
extern void _flush_dcache_all(void);
extern void _clear_dcache_all(void);
extern void _clear_dcache_range(unsigned long start, unsigned long end);
#define ARCH_IMPLEMENTS_FLUSH_DCACHE_PAGE 1
extern void flush_dcache_page(struct page *);

#define flush_cache_all()		_flush_cache_all()
#define flush_cache_mm(mm)		_flush_cache_mm(mm)

#define dmac_flush_range(start, end)	dma_cache_wback(start, end - start)
/*
 * as the CK610 cache is PIVT and is 16K, we will flush all
 * if the flush range is larger than a page(4K).
 *
 * it is hard to decide how large the scale should be flush all,
 * so, this conclusion(large than 4K) is reference to mips and arm.
 */
#ifndef CONFIG_CSKY_CACHE_LINE_FLUSH /* CSKY_CACHE_LINE_FLUSH */
#define flush_cache_range(mm,start,end)	_flush_cache_range(mm,start,end)
#define flush_cache_page(vma,page,pfn)	_flush_cache_page(vma, page)
#define flush_page_to_ram(page)		do { } while (0)

#ifdef  CONFIG_MMU
#define flush_icache_range(start, end)	_flush_icache_range(start,end)
#else
#define flush_icache_range(start, end)  _flush_cache_all()
#endif
#define flush_icache_user_range(vma, page, addr, len) \
					_flush_icache_page((vma), (page))
#define flush_icache_page(vma, page) 	_flush_icache_page(vma, page)

#define flush_icache_all()		_flush_icache_all()
#define flush_dcache_all()		_flush_dcache_all()
#define clear_dcache_all()		_clear_dcache_all()
#define clear_dcache_range(start, len)  _clear_dcache_range(start,start+len)

#define flush_cache_dup_mm(mm)                  flush_cache_mm(mm)
#define flush_dcache_mmap_lock(mapping)         do { } while (0)
#define flush_dcache_mmap_unlock(mapping)       do { } while (0)

#define flush_cache_vmap(start, end)            flush_cache_all()
#define flush_cache_vunmap(start, end)          flush_cache_all()

#else /* if define CSKY_CACHE_LINE_FLUSH */

/*
 * arguement:
 * start:first address, end:last address
 */
#ifdef CONFIG_CPU_CSKYV1
extern void __flush_all_range(unsigned long start, unsigned long end);
extern void __flush_icache_range(unsigned long start, unsigned long end);
extern void __flush_dcache_range(unsigned long start, unsigned long end);
#elif defined (CONFIG_CPU_CSKYV2)
extern void __flush_cache_range(unsigned long start, unsigned long end, unsigned long value);

#define __flush_icache_range(start,end)	\
__flush_cache_range(start, end, INS_CACHE | CACHE_INV)
#define __flush_dcache_range(start,end)	\
__flush_cache_range(start, end, DATA_CACHE | CACHE_INV | CACHE_CLR)
#define __flush_all_range(start,end)	\
__flush_cache_range(start, end, INS_CACHE | DATA_CACHE | CACHE_INV | CACHE_CLR)

#endif

#define flush_cache_range(mm,start,end) __flush_all_range(start,end)
#define flush_cache_page(vma,page,pfn)  _flush_cache_page(vma, page)
#define flush_page_to_ram(page)         do { } while (0)

#define flush_icache_range(start, end)  __flush_icache_range(start,end)
#define flush_icache_user_range(vma, page, addr, len) \
                                        __flush_icache_range(addr,addr+len)
#define flush_icache_page(vma, page)    _flush_icache_page(vma, page)
#define flush_icache_all()              _flush_icache_all()
#define flush_dcache_all()              _flush_dcache_all()
#define clear_dcache_all()              _clear_dcache_all()
#define clear_dcache_range(start, len) __flush_dcache_range(start,start+len);

#define flush_cache_dup_mm(mm)                  flush_cache_mm(mm)
#define flush_dcache_mmap_lock(mapping)         do { } while (0)
#define flush_dcache_mmap_unlock(mapping)       do { } while (0)

#define flush_cache_vmap(start, end)            __flush_all_range(start, end)
#define flush_cache_vunmap(start, end)          __flush_all_range(start, end)

#endif /* CSKY_CACHE_LINE_FLUSH */

static inline void copy_to_user_page(struct vm_area_struct *vma,
                                     struct page *page, unsigned long vaddr,
                                     void *dst, void *src, int len)
{
	flush_cache_page(vma, vaddr, page_to_pfn(page));
	memcpy(dst, src, len);
	flush_icache_user_range(vma, page, vaddr, len);
}
static inline void copy_from_user_page(struct vm_area_struct *vma,
                                       struct page *page, unsigned long vaddr,
                                       void *dst, void *src, int len)
{
	flush_cache_page(vma, vaddr, page_to_pfn(page));
	memcpy(dst, src, len);
}

#endif /* __ASM_CSKY_CACHEFLUSH_H */
