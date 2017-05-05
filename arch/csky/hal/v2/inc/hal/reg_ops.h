#ifndef __ASM_REG_OPS_H
#define __ASM_REG_OPS_H

static inline unsigned int mfcr_cpuidrr(void)
{
	unsigned int ret;
	asm volatile("mfcr %0, cr13" :"=r" (ret));
	return ret;
}

static inline void mtcr_hint(unsigned int value)
{
	asm volatile(
		"mtcr %0, cr31"
		::"r"(value));
}

static inline void mtcr_ccr(unsigned int value)
{
	asm volatile(
		"mtcr %0, cr18"
		::"r"(value));
}

#endif /* __ASM_REG_OPS_H */

