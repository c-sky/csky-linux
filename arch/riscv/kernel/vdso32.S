/* SPDX-License-Identifier: GPL-2.0-only */

#define	vdso64_start	vdso32_start
#define	vdso64_end	vdso32_end

#define	__VDSO_PATH	"arch/riscv/kernel/vdso/vdso32.so"

#include "vdso64.S"
