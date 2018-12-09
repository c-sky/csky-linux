// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd. */

#include <linux/export.h>
#include <linux/sched.h>
#include <linux/stacktrace.h>
#include <linux/thread_info.h>
#include <linux/ptrace.h>

void save_stack_trace(struct stack_trace *trace)
{
	save_stack_trace_tsk(NULL, trace);
}
EXPORT_SYMBOL_GPL(save_stack_trace);

extern int in_sched_functions(unsigned long addr);
void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
	unsigned long *fp, *stack_start, *stack_end;
	unsigned long addr;
	int skip = trace->skip;

	if (tsk == NULL)
		asm volatile("mov %0, r8\n":"=r"(fp));
	else
		fp = (unsigned long *)thread_saved_fp(tsk);

	addr = (unsigned long) fp & THREAD_MASK;
	stack_start = (unsigned long *) addr;
	stack_end = (unsigned long *) (addr + THREAD_SIZE);

	while (fp > stack_start && fp < stack_end) {
		unsigned long lr, fpp;

		fpp = fp[0];
		lr = fp[1];
		if (!__kernel_text_address(lr))
			break;

		if ((tsk == current) || !in_sched_functions(lr)) {
			if (skip) {
				skip--;
			} else {
				trace->entries[trace->nr_entries++] = lr;
				if (trace->nr_entries >= trace->max_entries)
					break;
			}
		}
		fp = (unsigned long *)fpp;
	}
}
EXPORT_SYMBOL_GPL(save_stack_trace_tsk);
