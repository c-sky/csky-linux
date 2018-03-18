// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/linkage.h>
#include <abi/entry.h>

.text

/*
 * int csky_cmpxchg(int oldval, int newval, int *ptr)
 *
 * If *ptr != oldval && return 1,
 * else *ptr = newval return 0.
 */
ENTRY(csky_cmpxchg)
	USPTOKSP
	mfcr	a3, epc
	INCTRAP	a3

	subi    sp, 8
	stw     a3, (sp, 0)
	mfcr    a3, epsr
	stw     a3, (sp, 4)

	psrset	ee
1:
	ldw	a3, (a2)
	cmpne	a0, a3
	bt16	3f
2:
	stw	a1, (a2)
3:
	mvc	a0
	ldw	a3, (sp, 0)
	mtcr	a3, epc
	ldw     a3, (sp, 4)
	mtcr	a3, epsr
	addi	sp, 8
	KSPTOUSP
	rte
END(csky_cmpxchg)

/*
 * Called from tlbmodified exception
 */
ENTRY(csky_cmpxchg_fixup)
	mfcr	a0, epc
	lrw	a1, 2b
	cmpne	a1, a0
	bt	1f
	subi	a1, (2b - 1b)
	stw	a1, (sp, 0)
1:
	rts
END(csky_cmpxchg_fixup)

