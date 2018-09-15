// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#ifndef __ABI_CSKY_TLB_H
#define __ABI_CSKY_TLB_H

#define tlb_start_vma(tlb, vma) \
	do { \
		if (!tlb->fullmm) \
			cache_wbinv_all(); \
	}  while (0)
#endif /* __ABI_CSKY_TLB_H */
