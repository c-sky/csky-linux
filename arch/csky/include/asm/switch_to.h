#ifndef __ASM_CSKY_SWITCH_TO_H
#define __ASM_CSKY_SWITCH_TO_H

#if defined(CONFIG_CPU_HAS_FPU) && defined(__CSKYABIV1__)
//FIXME: maybe death cycle auch as FPU error!
#define _is_fpu_busying()			\
	do {					\
		unsigned long fsr, flags;	\
		__asm__ __volatile__( \
			"mfcr    %0, psr\n\r"  \
			"cpseti  1   \n\r"     \
			"1:		"           \
			"cprsr   %1\n\r"       \
			"btsti   %1, 31\n\r"   \
			"bt      1b\n\r"       \
			"mtcr    %0, psr\n\r"  \
			:"=r"(flags), "=r"(fsr)\
		);                      \
	} while(0)
#else
#define _is_fpu_busying()  do { } while(0)
#endif

/*
 * switch_to(n) should switch tasks to task ptr, first checking that
 * ptr isn't the current task, in which case it does nothing.  This
 * also clears the TS-flag if the task we switched to has used the
 * math co-processor latest.
 *
 * syscall stores these registers itself and none of them are used
 * by syscall after the function in the syscall has been called.
 *
 * pass 'prev' in a0, 'next' in a1 and 'last' actually equal to 'prev'.
 */
asmlinkage void resume(void);
#define switch_to(prev,next,last)				\
	do {							\
		register void *_prev __asm__ ("a0") = (prev);	\
		register void *_next __asm__ ("a1") = (next);	\
		register void *_last __asm__ ("a0");		\
		_is_fpu_busying();				\
		__asm__ __volatile__(				\
			" jbsr  resume \n\t"			\
			: "=r"  (_last)				\
			: "r"   (_prev), "r" (_next)		\
			: "a2", "a3"				\
			);					\
		(last) = _last;					\
	} while(0)
#endif

