#ifndef __ASM_CSKY_OUTERCACHE_H
#define __ASM_CSKY_OUTERCACHE_H

#include <linux/compiler.h>
#include <asm/cache.h>

#ifdef CONFIG_CSKY_L2_CACHE
/*
 * Cache flushing:
 *  - flush_icache_range(start, end) flush a range of instructions
 */

extern void __flush_l2_all(unsigned long value);
extern void __flush_l2_cache_range(unsigned long start, unsigned long end, unsigned long value);
extern void __flush_l2_disable(void);

static inline void outer_inv_range(unsigned long start, unsigned long end){
	__flush_l2_cache_range(start, end, L2_CACHE_INV);
}

static inline void outer_clean_range(unsigned long start, unsigned long end){
	__flush_l2_cache_range(start, end, L2_CACHE_CLR);
}

static inline void outer_flush_range(unsigned long start, unsigned long end){
	__flush_l2_cache_range(start, end, L2_CACHE_CLR | L2_CACHE_INV);
}

static inline void outer_flush_all(void){
	__flush_l2_all(L2_CACHE_CLR | L2_CACHE_INV);
}

static inline void outer_inv_all(void){
	__flush_l2_all(L2_CACHE_INV);
}

static inline void outer_disable(void){
	__flush_l2_disable();
}

#endif

#endif /* __ASM_CSKY_OUTERCACHE_H */
