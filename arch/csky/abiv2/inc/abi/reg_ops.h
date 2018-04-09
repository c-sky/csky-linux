// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ABI_REG_OPS_H
#define __ABI_REG_OPS_H
#include <asm/reg_ops.h>

static inline unsigned int mfcr_hint(void)
{
	return mfcr("cr31");
}

static inline unsigned int mfcr_ccr2(void)
{
	return mfcr("cr23");
}

static inline unsigned int mfcr_msa0(void)
{
	return mfcr("cr<30, 15>");
}

static inline void mtcr_msa0(unsigned int value)
{
	mtcr("cr<30, 15>", value);
}

static inline unsigned int mfcr_msa1(void)
{
	return mfcr("cr<31, 15>");
}

static inline void mtcr_msa1(unsigned int value)
{
	mtcr("cr<31, 15>", value);
}

#endif /* __ABI_REG_OPS_H */

