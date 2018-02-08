#ifndef __ASM_REG_OPS_H
#define __ASM_REG_OPS_H

static inline unsigned int mfcr_cpuidrr(void)
{
	unsigned int ret;
	__asm__ __volatile__(
		"mfcr %0, cr13\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_hint(void)
{
	unsigned int ret;
	__asm__ __volatile__(
		"mfcr %0, cr30\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_ccr(void)
{
	unsigned int ret;
	__asm__ __volatile__(
		"mfcr %0, cr18\t\n"
		:"=r"(ret));
	return ret;
}

static inline unsigned int mfcr_msa0(void)
{
	unsigned int ret;
	__asm__ __volatile__(
		"cprcr %0, cpcr30\t\n"
		:"=r"(ret));
	return ret;
}

static inline void mtcr_msa0(unsigned int value)
{
	__asm__ __volatile__(
		"cpwcr %0, cpcr30\t\n"
		::"r"(value));
}

static inline unsigned int mfcr_msa1(void)
{
	unsigned int ret;
	__asm__ __volatile__(
		"cprcr %0, cpcr31\t\n"
		:"=r"(ret));
	return ret;
}

static inline void mtcr_msa1(unsigned int value)
{
	__asm__ __volatile__(
		"cpwcr %0, cpcr31\t\n"
		::"r"(value));
}

static inline unsigned int mfcr_ccr2(void){return 0;}

#endif /* __ASM_REG_OPS_H */

