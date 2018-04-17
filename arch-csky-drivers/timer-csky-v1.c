// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched_clock.h>
#include <asm/reg_ops.h>

#include "timer-of.h"

#define PTIM_CTLR	"cr<0, 14>"
#define PTIM_TSR	"cr<1, 14>"
#define PTIM_CCVR_HI	"cr<2, 14>"
#define PTIM_CCVR_LO	"cr<3, 14>"
#define PTIM_LVR	"cr<6, 14>"

#define BITS_CSKY_TIMER	56

DECLARE_PER_CPU(struct timer_of, csky_to);

static inline u64 get_ccvr(void)
{
	u32 lo, hi, t;

	do {
		hi = mfcr(PTIM_CCVR_HI);
		lo = mfcr(PTIM_CCVR_LO);
		t  = mfcr(PTIM_CCVR_HI);
	} while(t != hi);

	return ((u64)hi << 32) | lo;
}

static irqreturn_t timer_interrupt(int irq, void *dev)
{
	struct timer_of *to = this_cpu_ptr(&csky_to);

	mtcr(PTIM_TSR, 0);

	to->clkevt.event_handler(&to->clkevt);

	return IRQ_HANDLED;
}

static int csky_timer_set_next_event(unsigned long delta, struct clock_event_device *ce)
{
	mtcr(PTIM_LVR, delta);

	return 0;
}

static int csky_timer_shutdown(struct clock_event_device *ce)
{
	mtcr(PTIM_CTLR, 0);

	return 0;
}

static int csky_timer_oneshot(struct clock_event_device *ce)
{
	mtcr(PTIM_CTLR, 1);

	return 0;
}

static int csky_timer_oneshot_stopped(struct clock_event_device *ce)
{
	mtcr(PTIM_CTLR, 0);

	return 0;
}

DEFINE_PER_CPU(struct timer_of, csky_to) = {
	.flags = TIMER_OF_CLOCK | TIMER_OF_IRQ,

	.clkevt = {
		.name = "csky_timer_v1_clkevt",
		.rating = 400,
		.features = CLOCK_EVT_FEAT_PERCPU | CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown		= csky_timer_shutdown,
		.set_state_oneshot		= csky_timer_oneshot,
		.set_state_oneshot_stopped	= csky_timer_oneshot_stopped,
		.set_next_event			= csky_timer_set_next_event,
		.cpumask = cpu_possible_mask,
	},

	.of_irq = {
		.handler = timer_interrupt,
		.flags = IRQF_TIMER,
		.percpu = 0,
	},
};

static void csky_clkevt_init(int cpu_id, u32 hz)
{
	struct timer_of *to = per_cpu_ptr(&csky_to, cpu_id);

	clockevents_config_and_register(&to->clkevt, hz, 1, ULONG_MAX);
}

static u64 sched_clock_read(void)
{
	return get_ccvr();
}

static u64 clksrc_read(struct clocksource *c)
{
	return get_ccvr();
}

DEFINE_PER_CPU(struct clocksource, csky_clocksource) = {
	.name = "csky_timer_v1_clksrc",
	.rating = 400,
	.mask = CLOCKSOURCE_MASK(BITS_CSKY_TIMER),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
	.read = clksrc_read,
};

static void csky_clksrc_init(int cpu_id, u32 hz)
{
	struct clocksource *cs = per_cpu_ptr(&csky_clocksource, cpu_id);

	clocksource_register_hz(cs, hz);

	sched_clock_register(sched_clock_read, BITS_CSKY_TIMER, hz);
}

static int __init csky_timer_v1_init(struct device_node *np)
{
	int ret;
	int cpu_id = 0;
	struct timer_of *to;

	to = per_cpu_ptr(&csky_to, cpu_id);

	ret = timer_of_init(np, to);
	if (ret)
		return ret;

	csky_clkevt_init(cpu_id, timer_of_rate(to));

	csky_clksrc_init(cpu_id, timer_of_rate(to));

	return 0;
}
TIMER_OF_DECLARE(csky_timer_v1, "csky,timer-v1", csky_timer_v1_init);

