// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched_clock.h>
#include <linux/cpu.h>
#include <asm/reg_ops.h>

#include "timer-of.h"

#define PTIM_CCVR	"cr<3, 14>"
#define PTIM_CTLR	"cr<0, 14>"
#define PTIM_LVR	"cr<6, 14>"
#define PTIM_TSR	"cr<1, 14>"

static int csky_mptimer_set_next_event(unsigned long delta, struct clock_event_device *ce)
{
	mtcr(PTIM_LVR, delta);

	return 0;
}

static int csky_mptimer_shutdown(struct clock_event_device *ce)
{
	mtcr(PTIM_CTLR, 0);

	return 0;
}

static int csky_mptimer_oneshot(struct clock_event_device *ce)
{
	mtcr(PTIM_CTLR, 1);

	return 0;
}

static int csky_mptimer_oneshot_stopped(struct clock_event_device *ce)
{
	mtcr(PTIM_CTLR, 0);

	return 0;
}

static DEFINE_PER_CPU(struct timer_of, csky_to) = {
	.flags					= TIMER_OF_CLOCK,
	.clkevt = {
		.rating				= 300,
		.features			= CLOCK_EVT_FEAT_PERCPU |
						  CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown		= csky_mptimer_shutdown,
		.set_state_oneshot		= csky_mptimer_oneshot,
		.set_state_oneshot_stopped	= csky_mptimer_oneshot_stopped,
		.set_next_event			= csky_mptimer_set_next_event,
	},
	.of_irq = {
		.flags				= IRQF_TIMER,
		.percpu				= 1,
	},
};

static irqreturn_t timer_interrupt(int irq, void *dev)
{
	struct timer_of *to = this_cpu_ptr(&csky_to);

	mtcr(PTIM_TSR, 0);

	to->clkevt.event_handler(&to->clkevt);

	return IRQ_HANDLED;
}

/*
 * clock event for percpu
 */
static int csky_mptimer_starting_cpu(unsigned int cpu)
{
	struct timer_of *to = per_cpu_ptr(&csky_to, cpu);

	to->clkevt.cpumask = cpumask_of(cpu);

	clockevents_config_and_register(&to->clkevt, timer_of_rate(to), 2, ULONG_MAX);

	enable_percpu_irq(timer_of_irq(to), 0);

	return 0;
}

static int csky_mptimer_dying_cpu(unsigned int cpu)
{
	struct timer_of *to = per_cpu_ptr(&csky_to, cpu);

	disable_percpu_irq(timer_of_irq(to));

	return 0;
}

/*
 * clock source
 */
static u64 sched_clock_read(void)
{
	return (u64) mfcr(PTIM_CCVR);
}

static u64 clksrc_read(struct clocksource *c)
{
	return (u64) mfcr(PTIM_CCVR);
}

struct clocksource csky_clocksource = {
	.name	= "csky",
	.rating	= 400,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.read	= clksrc_read,
};

#define CPUHP_AP_CSKY_TIMER_STARTING CPUHP_AP_DUMMY_TIMER_STARTING

static int __init csky_mptimer_init(struct device_node *np)
{
	int ret, cpu;
	struct timer_of *to;
	int rate = 0;
	int irq	 = 0;

	/*
	 * Csky_mptimer is designed for C-SKY SMP multi-processors and
	 * every core has it's own private irq and regs for clkevt and
	 * clksrc.
	 *
	 * The regs is accessed by cpu instruction: mfcr/mtcr instead of
	 * mmio map style. So we needn't mmio-address in dts, but we still
	 * need to give clk and irq number.
	 *
	 * We use private irq for the mptimer and irq number is the same
	 * for every core. So we use request_percpu_irq() in timer_of_init.
	 */

	for_each_possible_cpu(cpu) {
		to = per_cpu_ptr(&csky_to, cpu);

		if (cpu == 0) {
			to->flags	  |= TIMER_OF_IRQ;
			to->of_irq.handler = timer_interrupt;

			ret = timer_of_init(np, to);
			if (ret)
				return ret;

			rate	= timer_of_rate(to);
			irq	= to->of_irq.irq;
		} else {
			ret = timer_of_init(np, to);
			if (ret)
				return ret;

			to->of_clk.rate	= rate;
			to->of_irq.irq	= irq;
		}
	}

	ret = cpuhp_setup_state(CPUHP_AP_CSKY_TIMER_STARTING,
				"clockevents/csky/timer:starting",
				csky_mptimer_starting_cpu,
				csky_mptimer_dying_cpu);
	if (ret) {
		pr_err("%s: Failed to cpuhp_setup_state.\n", __func__);
		return ret;
	}

	clocksource_register_hz(&csky_clocksource, rate);
	sched_clock_register(sched_clock_read, 32, rate);

	return 0;
}
TIMER_OF_DECLARE(csky_mptimer, "csky,mptimer", csky_mptimer_init);
