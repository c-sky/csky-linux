// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/cacheflush.h>

/*
 * Defined in libgcc
 *
 * See arch/csky/Makefile:
 *	-print-libgcc-file-name
 */
extern void __ashldi3 (void);
extern void __ashrdi3 (void);
extern void __lshrdi3 (void);
extern void __muldi3 (void);
extern void __ucmpdi2 (void);
EXPORT_SYMBOL(__ashldi3);
EXPORT_SYMBOL(__ashrdi3);
EXPORT_SYMBOL(__lshrdi3);
EXPORT_SYMBOL(__muldi3);
EXPORT_SYMBOL(__ucmpdi2);

/*
 * Defined in abiv1/src/memcpy.S
 * and abiv2/src/memcpy.c
 */
EXPORT_SYMBOL(memcpy);

/* Defined in lib/memset.c */
EXPORT_SYMBOL(memset);
