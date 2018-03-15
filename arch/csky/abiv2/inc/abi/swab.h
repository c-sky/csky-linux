#ifndef __ABI_CSKY_SWAB_H
#define __ABI_CSKY_SWAB_H

#include <linux/compiler.h>
#include <linux/types.h>

static inline __attribute_const__ __u16 __arch_swab16(__u16 x)
{
	asm("revh %0, %1\n":"=r" (x):"r"(x));
	return x;
}
#define __arch_swab16 __arch_swab16

static inline __attribute_const__ __u32 __arch_swab32(__u32 x)
{
	asm("revb %0, %1\n":"=r"(x):"r"(x));
	return x;
}
#define __arch_swab32 __arch_swab32

#endif /* __ABI_CSKY_SWAB_H */
