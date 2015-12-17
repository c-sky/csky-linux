#ifndef __ASM_CSKY_MMU_H
#define __ASM_CSKY_MMU_H

#ifdef CONFIG_MMU
	typedef struct {
	    unsigned long asid[NR_CPUS];
	    void *vdso;
	} mm_context_t;
#else
	typedef struct {
		unsigned long		end_brk;
	} mm_context_t;
#endif

#endif /* __ASM_CSKY_MMU_H */
