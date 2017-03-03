#ifndef __ASM_CSKY_SWITCH_TO_H
#define __ASM_CSKY_SWITCH_TO_H

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
		__asm__ __volatile__(				\
			" jbsr  resume \n\t"			\
			: "=r"  (_last)				\
			: "r"   (_prev), "r" (_next)		\
			: "a2", "a3"				\
			);					\
		(last) = _last;					\
	} while(0)
#endif

