#ifndef __ASM_CSKY_IRQFLAGS_H
#define __ASM_CSKY_IRQFLAGS_H

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;
	__asm__ __volatile__(
		"mfcr	%0, psr	\n"
		"psrclr	ie	\n"
		:"=r"(flags)
		:
		:
		);
	return flags;
}
#define arch_local_irq_save arch_local_irq_save

static inline void arch_local_irq_enable(void)
{
	__asm__ __volatile__(
		"psrset ee, ie \n"
		:
		:
		:
		);
}
#define arch_local_irq_enable arch_local_irq_enable

static inline void arch_local_irq_disable(void)
{
	__asm__ __volatile__(
		"psrclr ie \n"
		:
		:
		:
		);
}
#define arch_local_irq_disable arch_local_irq_disable

static inline unsigned long arch_local_save_flags(void)
{
	unsigned long flags;
	__asm__ __volatile__(
		"mfcr	%0, psr	\n"
		:"=r"(flags)
		:
		:
		);
	return flags;
}
#define arch_local_save_flags arch_local_save_flags

static inline void arch_local_irq_restore(unsigned long flags)
{
	__asm__ __volatile__(
		"mtcr 	%0, psr \n"
		:
		:"r" (flags)
		:"memory"
		);
}
#define arch_local_irq_restore arch_local_irq_restore

static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !(flags & (1<<6));
}
#define arch_irqs_disabled_flags arch_irqs_disabled_flags

#include <asm-generic/irqflags.h>

#endif /* __ASM_CSKY_IRQFLAGS_H */
