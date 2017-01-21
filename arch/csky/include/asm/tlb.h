#ifndef __ASM_CSKY_TLB_H
#define __ASM_CSKY_TLB_H

#include <asm/cacheflush.h>

#define tlb_start_vma(tlb, vma) \
	do { \
		if (!tlb->fullmm) \
		cache_op_range(vma->vm_start, vma->vm_end, \
			INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV); \
	}  while (0)

#if defined(CONFIG_MMU_HARD_REFILL) && !defined(__ck807__)
/*
 * FIXME: may be use function flush_tlb_range like other arch.
 */
#define tlb_end_vma(tlb, vma) \
	do { \
		if (!tlb->fullmm) \
		cache_op_range(vma->vm_start, vma->vm_end, \
			INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV); \
	}  while (0)
#endif

#define tlb_flush(tlb)	flush_tlb_mm((tlb)->mm)

#include <asm-generic/tlb.h>

#endif /* __ASM_CSKY_TLB_H */
