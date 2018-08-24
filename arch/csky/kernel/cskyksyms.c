// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/module.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,18,0)
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
#endif
