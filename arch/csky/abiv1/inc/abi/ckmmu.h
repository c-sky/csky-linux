// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_CKMMUV1_H
#define __ASM_CSKY_CKMMUV1_H

static inline void select_mmu_cp(void)
{
	 asm volatile("cpseti cp15\n");
}

static inline int read_mmu_index(void)
{
	int res;
	asm volatile(
		"cprcr %0, cpcr0\n"
		:"=b" (res));
	return   res;
}

static inline void write_mmu_index(int value)
{

	asm volatile(
		"cpwcr %0, cpcr0\n"
		::"b"(value));
}

static inline int read_mmu_entrylo0(void)
{
	int res;
	asm volatile(
		"cprcr %0, cpcr2\n"
		:"=b" (res));

	return res << 6;
}

static inline int read_mmu_entrylo1(void)
{
	int res;
	asm volatile(
		"cprcr %0, cpcr3\n"
		:"=b" (res));

	return res << 6;
}

static inline void write_mmu_pagemask(int value)
{
	asm volatile(
		"cpwcr %0, cpcr6\n"
		::"b"(value));
}

static inline int read_mmu_entryhi(void)
{
	int res;
	asm volatile(
		"cprcr %0, cpcr4\n"
		:"=b" (res));

	return res;
}

static inline void write_mmu_entryhi(int value)
{

	asm volatile(
		"cpwcr %0, cpcr4\n"
		::"b"(value));
}

/*
 * TLB operations.
 */
static inline void tlb_probe(void)
{
	int value = 0x80000000;

	asm volatile(
		"cpwcr %0, cpcr8\n"
		::"b"(value));
}

static inline void tlb_read(void)
{
	int value = 0x40000000;

	asm volatile(
		"cpwcr %0, cpcr8\n"
		::"b"(value));
}

static inline void tlb_invalid_all(void)
{
	int value = 0x04000000;

	asm volatile(
		"cpwcr %0, cpcr8\n"
		::"b"(value));
}

static inline void tlb_invalid_indexed(void)
{
	int value = 0x02000000;

	asm volatile(
		"cpwcr %0, cpcr8\n"
		::"b"(value));
}

/* misc */
static inline void tlbmiss_handler_setup_pgd(unsigned long pgd)
{
	asm volatile(
		"bseti %0, 0		\n"
		"bclri %0, 31		\n"
		"addu  %0, %1		\n"
		"cpseti cp15		\n"
		"cpwcr %0, cpcr29	\n"
		::"b"(pgd), "r"(PHYS_OFFSET)
		:);
}

static inline unsigned long tlb_get_pgd(void)
{
	unsigned long pgd;
	asm volatile(
		"cpseti	cp15		\n"
		"cprcr	%0, cpcr29	\n"
		"bclri	%0, 0		\n"
		"subu	%0, %1		\n"
                "bseti	%0, 31		\n"
                :"=&b"(pgd)
		:"r"(PHYS_OFFSET)
                :);
	return pgd;
}
#endif /* __ASM_CSKY_CKMMUV1_H */

