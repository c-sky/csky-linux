/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/ftrace.h>
#include <linux/uaccess.h>

#ifdef CONFIG_FUNCTION_GRAPH_TRACER
void prepare_ftrace_return(unsigned long *parent, unsigned long self_addr,
			   unsigned long frame_pointer)
{
	unsigned long return_hooker = (unsigned long)&return_to_handler;
	struct ftrace_graph_ent trace;
	unsigned long old;
	int err;

	if (unlikely(atomic_read(&current->tracing_graph_pause)))
		return;

	old = *parent;

	trace.func = self_addr;
	trace.depth = current->curr_ret_stack + 1;

	/* Only trace if the calling function expects to */
	if (!ftrace_graph_entry(&trace)) {
		return;
	}

	err = ftrace_push_return_trace(old, self_addr, &trace.depth,
				       *(unsigned long *)frame_pointer, NULL);
	if (err == -EBUSY) {
		return;
	} else {
		/*
		 * For csky-gcc function has sub-call:
		 * subi	sp,	sp, 8
		 * stw	r8,	(sp, 0)
		 * mov	r8,	sp
		 * st.w r15,	(sp, 0x4)
		 * push	r15
		 * jl	_mcount
		 * We only need set *parent for resume
		 *
		 * For csky-gcc function has no sub-call:
		 * subi	sp,	sp, 4
		 * stw	r8,	(sp, 0)
		 * mov	r8,	sp
		 * push	r15
		 * jl	_mcount
		 * We need set *parent and *(frame_pointer + 4) for resume,
		 * because lr is resumed twice.
		 */
		*parent = return_hooker;
		frame_pointer += 4;
		if (*(unsigned long *)frame_pointer == old)
			*(unsigned long *)frame_pointer = return_hooker;
	}
}
#endif

/* _mcount is defined in abi's mcount.S */
extern void _mcount(void);
EXPORT_SYMBOL(_mcount);
