#ifndef __ASM_CSKY_SWAB_H
#define __ASM_CSKY_SWAB_H

#include <linux/compiler.h>
#include <linux/types.h>

#ifdef CONFIG_CPU_CSKYV2

static inline __attribute_const__ __u16 __arch_swab16(__u16 x)
{
	__asm__ ("revh %0, %1" : "=r" (x) : "r" (x));
	return x;
}
#define __arch_swab16 __arch_swab16

static inline __attribute_const__ __u32 __arch_swab32(__u32 x)
{
	__asm__ ("revb %0, %1" : "=r" (x) : "r" (x));
	return x;
}
#define __arch_swab32 __arch_swab32

#endif /* CONFIG_CPU_CSKYV2 */

#endif /* __ASM_CSKY_SWAB_H */
