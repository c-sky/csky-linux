/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_RWONCE_H
#define __ASM_RWONCE_H

#include <linux/compiler_types.h>
#include <asm/alternative-macros.h>
#include <asm/vendorid_list.h>

#define __WRITE_ONCE(x, val)				\
do {							\
	*(volatile typeof(x) *)&(x) = (val);		\
	asm volatile(ALTERNATIVE(			\
		__nops(1),				\
		"fence w, o\n\t",			\
		THEAD_VENDOR_ID,			\
		ERRATA_THEAD_WRITE_ONCE,		\
		CONFIG_ERRATA_THEAD_WRITE_ONCE)		\
		: : : "memory");			\
} while (0)

#include <asm-generic/rwonce.h>

#endif	/* __ASM_RWONCE_H */
