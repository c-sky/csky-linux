#ifndef __ASM_CSKY_TLB_H
#define __ASM_CSKY_TLB_H

#include <asm/cacheflush.h>

#ifndef CONFIG_MMU
#error "forget uClinux!!"
#endif

#define tlb_start_vma(tlb, vma) \
	do { \
		if (!tlb->fullmm) \
		flush_cache_range(vma, vma->vm_start, vma->vm_end); \
	}  while (0)

#ifdef CONFIG_MMU_HARD_REFILL
/*
 * FIXME: may be use function flush_tlb_range like other arch.
 */
#define tlb_end_vma(tlb, vma) \
	do { \
		if (!tlb->fullmm) \
		clear_dcache_range(vma->vm_start, \
		vma->vm_end - vma->vm_start); \
	}  while (0)
#endif

/*
 * .. because we flush the whole mm when it
 * fills up.
 */
#define tlb_flush(tlb)	flush_tlb_mm((tlb)->mm)

#include <asm-generic/tlb.h>

#endif /* __ASM_CSKY_TLB_H */
