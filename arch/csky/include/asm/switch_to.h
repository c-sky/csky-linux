#ifndef __ASM_CSKY_SWITCH_TO_H
#define __ASM_CSKY_SWITCH_TO_H

#include <linux/thread_info.h>

/*
 * TODO: we need change switch_to with thread_info,
 * and we must move pt_regs from thread to thread_info first.
 */
extern struct task_struct *__switch_to(struct task_struct *, struct task_struct *);

#define switch_to(prev,next,last) \
do{ \
	last = __switch_to(prev, next); \
} while (0)

#endif /*__ASM_CSKY_SWITCH_TO_H */

