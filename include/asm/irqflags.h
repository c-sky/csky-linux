#ifndef __ASM_CSKY_IRQFLAGS_H
#define __ASM_CSKY_IRQFLAGS_H

/*
 * CPU interrupt mask handling.
 */
#define arch_local_irq_save __arch_local_irq_save
static inline unsigned long __arch_local_irq_save(void)
{
	unsigned long flags;
	asm volatile(
		"mfcr   %0, psr	\n"
		"psrclr ie      \n"
		:"=r"(flags): :"memory");
	return flags;
}

#define arch_local_irq_enable __arch_local_irq_enable
static inline void __arch_local_irq_enable(void)
{
	asm volatile(
		"psrset ee, ie \n"
		: : :"memory" );
}

#define arch_local_irq_disable __arch_local_irq_disable
static inline void __arch_local_irq_disable(void)
{
	asm volatile(
		"psrclr ie     \n"
		: : :"memory");
}

/*
 * Save the current interrupt enable state.
 */
#define arch_local_save_flags __arch_local_save_flags
static inline unsigned long __arch_local_save_flags(void)
{
	unsigned long flags;
	asm volatile(
		"mfcr   %0, psr	 \n"
		:"=r"(flags) : :"memory" );
	return flags;
}

/*
 * restore saved IRQ state
 */
#define arch_local_irq_restore __arch_local_irq_restore
static inline void __arch_local_irq_restore(unsigned long flags)
{
	asm volatile(
		"mtcr    %0, psr  \n"
		: :"r" (flags) :"memory" );
}

#define arch_irqs_disabled_flags __arch_irqs_disabled_flags
static inline int __arch_irqs_disabled_flags(unsigned long flags)
{
	return !((flags) & 0x40);
}

#include <asm-generic/irqflags.h>

#endif /* __ASM_CSKY_IRQFLAGS_H */
