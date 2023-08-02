// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c), 2023 Alibaba Cloud
 * Authors:
 *	Guo Ren <guoren@linux.alibaba.com>
 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM paravirt

#if !defined(_TRACE_PARAVIRT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_PARAVIRT_H

#include <linux/tracepoint.h>

TRACE_EVENT(pv_kick,
	TP_PROTO(int cpu, int target),
	TP_ARGS(cpu, target),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(int, target)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->target = target;
	),

	TP_printk("cpu %d kick target cpu %d",
		__entry->cpu,
		__entry->target
	)
);

TRACE_EVENT(pv_wait,
	TP_PROTO(int cpu),
	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(int, cpu)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
	),

	TP_printk("cpu %d out of wfi",
		__entry->cpu
	)
);

#endif /* _TRACE_PARAVIRT_H || TRACE_HEADER_MULTI_READ */

#undef TRACE_INCLUDE_PATH
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_PATH ../../../arch/riscv/kernel/
#define TRACE_INCLUDE_FILE trace_events_filter_paravirt

/* This part must be outside protection */
#include <trace/define_trace.h>
