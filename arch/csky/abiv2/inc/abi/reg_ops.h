// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ABI_REG_OPS_H
#define __ABI_REG_OPS_H

static inline unsigned int mfcr_hint(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr31\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_ccr2(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr23\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_msa0(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr<30, 15>\n"
		:"=r"(ret));
	return ret;
}

static inline void mtcr_msa0(unsigned int value)
{
	asm volatile(
		"mtcr %0, cr<30, 15>\n"
		::"r"(value));
}

static inline unsigned int mfcr_msa1(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr<31, 15>\n"
		:"=r"(ret));
	return ret;
}

static inline void mtcr_msa1(unsigned int value)
{
	asm volatile(
		"mtcr %0, cr<31, 15>\n"
		::"r"(value));
}

/* read/write user stack pointer */
static inline unsigned long rdusp(void) {
	register unsigned long usp;
	asm volatile("mfcr %0, cr<14, 1>\n":"=r" (usp));
	return usp;
}

static inline void wrusp(unsigned long usp) {
	asm volatile("mtcr %0, cr<14, 1>\n"::"r" (usp));
}

#endif /* __ABI_REG_OPS_H */

