// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_CKMMUV2_H
#define __ASM_CSKY_CKMMUV2_H

static inline void select_mmu_cp(void){}

static inline int  read_mmu_index(void)
{
	int res;
	asm volatile("mfcr %0,cr<0, 15>\n"
		:"=r"(res));
	return res;
}

static inline void  write_mmu_index(int value)
{
	asm volatile("mtcr %0,cr<0, 15>\n"
		::"r" (value));
}

static inline int  read_mmu_entrylo0(void)
{
	int res;
	asm volatile("mfcr %0,cr<2, 15>\n"
		:"=r"(res));
	return res;
}

static inline int  read_mmu_entrylo1(void)
{
	int res;

	asm volatile("mfcr %0,cr<3, 15>\n"
		:"=r"(res));
	return res;
}

static inline void  write_mmu_pagemask(int value)
{
	asm volatile("mtcr %0,cr<6, 15>\n"
		::"r" (value));
}

static inline int  read_mmu_entryhi(void)
{
	int res;

	asm volatile("mfcr %0,cr<4, 15>\n"
		:"=r" (res));
	return res;
}

static inline void  write_mmu_entryhi(int value)
{
	asm volatile("mtcr %0,cr<4, 15>\n"
		::"r" (value));
}

/*
 * TLB operations.
 */
static inline void tlb_probe(void)
{
	int value = 0x80000000;

	asm volatile("mtcr %0,cr<8, 15>\n"
		::"r" (value));
}

static inline void tlb_read(void)
{
	int value = 0x40000000;

	asm volatile("mtcr %0,cr<8, 15>\n"
		::"r" (value));
}

static inline void tlb_invalid_all(void)
{
#ifdef CONFIG_CPU_HAS_TLBI
	asm volatile("tlbi.all\n");
#else
	int value = 0x04000000;

	asm volatile("mtcr %0,cr<8, 15>\n"
		::"r" (value));
#endif
}

static inline void tlb_invalid_indexed(void)
{
	int value = 0x02000000;

	asm volatile("mtcr %0,cr<8, 15>\n"
		::"r" (value));
}

/* misc */
static inline void tlbmiss_handler_setup_pgd(unsigned long pgd)
{
	asm volatile(
		"bseti %0, 0		\n"
		"bclri %0, 31		\n"
		"addu  %0, %1		\n"
		"mtcr  %0, cr<29, 15>	\n"
		"mtcr  %0, cr<28, 15>	\n"
		::"r"(pgd), "r"(PHYS_OFFSET)
		:);
}

static inline unsigned long tlb_get_pgd(void)
{
	unsigned long pgd;
	asm volatile(
		"mfcr %0, cr<29, 15>	\n"
		"bclri	%0, 0		\n"
		"subu	%0, %1		\n"
                "bseti	%0, 31		\n"
                :"=&r"(pgd)
		:"r"(PHYS_OFFSET)
                :);
	return pgd;
}
#endif /* __ASM_CSKY_CKMMUV2_H */

