// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched_clock.h>
#include <asm/reg_ops.h>

#include "timer-of.h"

#define TIMER_NAME	"csky_timer_v1"

#define PTIM_CTLR	"cr<0, 14>"
#define PTIM_TSR	"cr<1, 14>"
#define PTIM_CCVR_HI	"cr<2, 14>"
#define PTIM_CCVR_LO	"cr<3, 14>"
#define PTIM_LVR	"cr<6, 14>"

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
	struct clock_event_device *ce = (struct clock_event_device *) dev;

	mtcr(PTIM_TSR, 0);

	ce->event_handler(ce);

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

static struct timer_of to = {
	.flags = TIMER_OF_CLOCK,

	.clkevt = {
		.name = TIMER_NAME,
		.rating = 200,
		.features = CLOCK_EVT_FEAT_DYNIRQ | CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown	= csky_timer_shutdown,
		.set_next_event		= csky_timer_set_next_event,
		.cpumask = cpu_possible_mask,
	},
};

static u64 notrace csky_sched_clock_read(void)
{
	return get_ccvr();
}

static void csky_clkevt_init(void)
{

	clockevents_config_and_register(&to.clkevt, TIMER_FREQ, 1, ULONG_MAX);
}

static u64 csky_timer_v2_clksrc_read(struct clocksource *c)
{
	return get_ccvr();
}

static void csky_clksrc_init(void)
{
	clocksource_mmio_init(NULL, "nationalchip", TIMER_FREQ, 400, 64,
			      csky_timer_v2_clksrc_read);

	sched_clock_register(csky_sched_clock_read, 32, TIMER_FREQ);
}

static int __init csky_timer_init(struct device_node *np)
{
	int ret;

	ret = timer_of_init(np, &to);
	if (ret)
		return ret;

	csky_clkevt_init();

	csky_clksrc_init();

	return 0;
}
TIMER_OF_DECLARE(csky_timer_v1, "csky,timer-v1", csky_timer_v1_init);

