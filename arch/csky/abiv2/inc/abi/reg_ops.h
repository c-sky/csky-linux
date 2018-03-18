// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_REG_OPS_H
#define __ASM_REG_OPS_H

static inline unsigned int mfcr_cpuidrr(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr13\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_hint(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr31\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_ccr(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr18\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_ccr2(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr23\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_msa0(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr<30, 15>\t\n"
		:"=r"(ret));
	return ret;
}

static inline void mtcr_msa0(unsigned int value)
{
	asm volatile(
		"mtcr %0, cr<30, 15>\t\n"
		::"r"(value));
}

static inline unsigned int mfcr_msa1(void)
{
	unsigned int ret;
	asm volatile(
		"mfcr %0, cr<31, 15>\t\n"
		:"=r"(ret));
	return ret;
}

static inline void mtcr_msa1(unsigned int value)
{
	asm volatile(
		"mtcr %0, cr<31, 15>\t\n"
		::"r"(value));
}

/* read/write user stack pointer */
static inline unsigned long rdusp(void) {
	register unsigned long usp;
	asm volatile("mfcr %0, cr<14, 1> \n\r":"=r" (usp));
	return usp;
}

static inline void wrusp(unsigned long usp) {
	asm volatile("mtcr %0, cr<14, 1> \n\r"::"r" (usp));
}

#endif /* __ASM_REG_OPS_H */

