#ifndef __ASM_REGS_OPS_H
#define __ASM_REGS_OPS_H
#include <abi/regdef.h>
#include <abi/reg_ops.h>

#define mfcr(reg)					\
({							\
	unsigned int tmp;				\
	asm volatile("mfcr %0, "#reg"\n":"=r"(tmp));	\
	tmp;						\
})

#define mtcr(reg, val)					\
({							\
	asm volatile("mtcr %0, "#reg"\n"::"r"(val));	\
})

#endif /* __ASM_REGS_OPS_H */
