#ifndef __ASM_CSKY_BARRIER_H
#define __ASM_CSKY_BARRIER_H

#ifndef __ASSEMBLY__

#define nop()	__asm__ __volatile__ ("nop")
#define mb()	__asm__ __volatile__ ("sync" :::"memory")

#include <asm-generic/barrier.h>

#endif /* __ASSEMBLY__ */
#endif /* __ASM_CSKY_BARRIER_H */
