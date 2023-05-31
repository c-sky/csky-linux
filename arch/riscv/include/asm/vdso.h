/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 ARM Limited
 * Copyright (C) 2014 Regents of the University of California
 * Copyright (C) 2017 SiFive
 */

#ifndef _ASM_RISCV_VDSO_H
#define _ASM_RISCV_VDSO_H

/*
 * All systems with an MMU have a VDSO, but systems without an MMU don't
 * support shared libraries and therefore don't have one.
 */
#ifdef CONFIG_MMU

#define __VVAR_PAGES    2

#ifndef __ASSEMBLY__

#ifdef CONFIG_VDSO64
#include <generated/vdso64-offsets.h>

#define VDSO64_SYMBOL(base, name)					\
	(void __user *)((unsigned long)(base) + rv64__vdso_##name##_offset)

extern char vdso64_start[], vdso64_end[];

#endif /* CONFIG_VDSO64 */

#ifdef CONFIG_VDSO32
#include <generated/vdso32-offsets.h>

#define VDSO32_SYMBOL(base, name)					\
	(void __user *)((unsigned long)(base) + rv32__vdso_##name##_offset)

extern char vdso32_start[], vdso32_end[];

#endif /* CONFIG_VDSO32 */

#ifdef CONFIG_VDSO64ILP32
#include <generated/vdso64ilp32-offsets.h>

#define VDSO64ILP32_SYMBOL(base, name)					\
	(void __user *)((unsigned long)(base) + rv64ilp32__vdso_##name##_offset)

extern char vdso64ilp32_start[], vdso64ilp32_end[];

#endif /* CONFIG_VDSO64ILP32 */

#endif /* !__ASSEMBLY__ */

#endif /* CONFIG_MMU */

#endif /* _ASM_RISCV_VDSO_H */
