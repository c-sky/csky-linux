// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched_clock.h>
#include <linux/cpu.h>
#include <asm/reg_ops.h>

#include "timer-of.h"

#define PTIM_CTLR	"cr<0, 14>"
#define PTIM_TSR	"cr<1, 14>"
#define PTIM_CCVR_HI	"cr<2, 14>"
#define PTIM_CCVR_LO	"cr<3, 14>"
#define PTIM_LVR	"cr<6, 14>"

#define BITS_CSKY_TIMER	56

DECLARE_PER_CPU(struct timer_of, csky_to);

static int csky_timer_irq;
static int csky_timer_rate;

static inline u64 get_ccvr(void)
{
	u32 lo, hi, t;

	t  = mfcr(PTIM_CCVR_LO);
	hi = mfcr(PTIM_CCVR_HI);
	lo = mfcr(PTIM_CCVR_LO);

	if (lo < t) hi++;

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
		.name = "C-SKY SMP Timer V1",
		.rating = 300,
		.features = CLOCK_EVT_FEAT_PERCPU | CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown		= csky_timer_shutdown,
		.set_state_oneshot		= csky_timer_oneshot,
		.set_state_oneshot_stopped	= csky_timer_oneshot_stopped,
		.set_next_event			= csky_timer_set_next_event,
	},

	.of_irq = {
		.handler = timer_interrupt,
		.flags = IRQF_TIMER,
		.percpu = 1,
	},
};

/*** clock event for percpu ***/
static int csky_timer_starting_cpu(unsigned int cpu)
{
	struct timer_of *to = this_cpu_ptr(&csky_to);

	to->clkevt.cpumask = cpumask_of(smp_processor_id());

	clockevents_config_and_register(&to->clkevt, csky_timer_rate, 0, ULONG_MAX);

	enable_percpu_irq(csky_timer_irq, 0);

	return 0;
}

static int csky_timer_dying_cpu(unsigned int cpu)
{
	disable_percpu_irq(csky_timer_irq);

	return 0;
}

/*** clock source ***/
static u64 sched_clock_read(void)
{
	return get_ccvr();
}

static u64 clksrc_read(struct clocksource *c)
{
	return get_ccvr();
}

struct clocksource csky_clocksource = {
	.name = "csky_timer_v1_clksrc",
	.rating = 400,
	.mask = CLOCKSOURCE_MASK(BITS_CSKY_TIMER),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
	.read = clksrc_read,
};

static void csky_clksrc_init(void)
{
	clocksource_register_hz(&csky_clocksource, csky_timer_rate);

	sched_clock_register(sched_clock_read, BITS_CSKY_TIMER, csky_timer_rate);
}

static int __init csky_timer_v1_init(struct device_node *np)
{
	int ret;
	struct timer_of *to = this_cpu_ptr(&csky_to);

	ret = timer_of_init(np, to);
	if (ret)
		return ret;

	csky_timer_irq = to->of_irq.irq;
	csky_timer_rate = timer_of_rate(to);

	ret = cpuhp_setup_state(CPUHP_AP_DUMMY_TIMER_STARTING,
				"clockevents/csky/timer:starting",
				csky_timer_starting_cpu,
				csky_timer_dying_cpu);
	if (ret) {
		pr_err("%s: Failed to cpuhp_setup_state.\n", __func__);
		return ret;
	}

	csky_clksrc_init();

	return ret;
}
TIMER_OF_DECLARE(csky_timer_v1, "csky,timer-v1", csky_timer_v1_init);

