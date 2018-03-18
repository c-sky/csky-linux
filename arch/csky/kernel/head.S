// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>
#include <abi/regdef.h>

ENTRY(_start)
	/* set super user mode */
	lrw	a3, DEFAULT_PSR_VALUE
	mtcr    a3, psr
	psrset  ee

	/* set stack point */
	lrw     a3, init_thread_union + THREAD_SIZE
	mov	sp, a3

	jmpi	csky_start
END(_start)

