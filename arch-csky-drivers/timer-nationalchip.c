// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched_clock.h>

#include "timer-of.h"

#define TIMER_NAME	"nc_timer"
#define TIMER_FREQ	1000000
#define CLKSRC_OFFSET	0x40

#define TIMER_STATUS	0x00
#define TIMER_VALUE	0x04
#define TIMER_CONTRL	0x10
#define TIMER_CONFIG	0x20
#define TIMER_DIV	0x24
#define TIMER_INI	0x28

#define STATUS_clr	BIT(0)

#define CONTRL_rst	BIT(0)
#define CONTRL_start	BIT(1)

#define CONFIG_en	BIT(0)
#define CONFIG_irq_en	BIT(1)

static irqreturn_t timer_interrupt(int irq, void *dev)
{
	struct clock_event_device *ce = (struct clock_event_device *) dev;
	void __iomem *base = timer_of_base(to_timer_of(ce));

	writel_relaxed(STATUS_clr, base + TIMER_STATUS);

	ce->event_handler(ce);

	return IRQ_HANDLED;
}

static int nc_timer_set_periodic(struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	/* reset */
	writel_relaxed(CONTRL_rst, base + TIMER_CONTRL);

	/* config the timeout value */
	writel_relaxed(ULONG_MAX - timer_of_period(to_timer_of(ce)),
		       base + TIMER_INI);

	/* enable with irq and start */
	writel_relaxed(CONFIG_en|CONFIG_irq_en, base + TIMER_CONFIG);
	writel_relaxed(CONTRL_start, base + TIMER_CONTRL);

	return 0;
}

static int nc_timer_set_next_event(unsigned long delta, struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	/* use reset to pause timer */
	writel_relaxed(CONTRL_rst, base + TIMER_CONTRL);

	/* config next timeout value */
	writel_relaxed(ULONG_MAX - delta, base + TIMER_INI);
	writel_relaxed(CONTRL_start, base + TIMER_CONTRL);

	return 0;
}

static int nc_timer_shutdown(struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	writel_relaxed(0, base + TIMER_CONTRL);
	writel_relaxed(0, base + TIMER_CONFIG);

	return 0;
}

static struct timer_of to = {
	.flags = TIMER_OF_IRQ | TIMER_OF_BASE | TIMER_OF_CLOCK,

	.clkevt = {
		.name = TIMER_NAME,
		.rating = 300,
		.features = CLOCK_EVT_FEAT_DYNIRQ | CLOCK_EVT_FEAT_PERIODIC |
			CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown	= nc_timer_shutdown,
		.set_state_periodic	= nc_timer_set_periodic,
		.set_next_event		= nc_timer_set_next_event,
		.cpumask = cpu_possible_mask,
	},

	.of_irq = {
		.handler = timer_interrupt,
		.flags = IRQF_TIMER | IRQF_IRQPOLL,
	},
};

static u64 notrace nc_sched_clock_read(void)
{
	void __iomem *base;

	base = timer_of_base(&to) + CLKSRC_OFFSET; 

	return (u64) readl_relaxed(base + TIMER_VALUE);
}

static void nc_timer_set_div(void __iomem *base)
{
	unsigned int div;

	div = timer_of_rate(&to)/TIMER_FREQ - 1;

	writel_relaxed(div, base + TIMER_DIV);
}

static void nc_clkevt_init(void __iomem *base)
{
	/* reset */
	writel_relaxed(CONTRL_rst, base + TIMER_CONTRL);

	/* reset config */
	writel_relaxed(0, base + TIMER_CONFIG);

	nc_timer_set_div(base);

	clockevents_config_and_register(&to.clkevt, TIMER_FREQ, 1, ULONG_MAX);
}

static void nc_clksrc_init(void __iomem *base)
{
	writel_relaxed(CONTRL_rst, base + TIMER_CONTRL);

	writel_relaxed(CONFIG_en, base + TIMER_CONFIG);

	nc_timer_set_div(base);

	writel_relaxed(0, base + TIMER_INI);
	writel_relaxed(CONTRL_start, base + TIMER_CONTRL);

	clocksource_mmio_init(base + TIMER_VALUE, "nationalchip", TIMER_FREQ, 200, 32,
			      clocksource_mmio_readl_up);

	sched_clock_register(nc_sched_clock_read, 32, TIMER_FREQ);
}

static int __init nc_timer_init(struct device_node *np)
{
	int ret;

	ret = timer_of_init(np, &to);
	if (ret)
		return ret;

	nc_clkevt_init(timer_of_base(&to));

	nc_clksrc_init(timer_of_base(&to) + CLKSRC_OFFSET);

	return 0;
}
TIMER_OF_DECLARE(nc_timer, "nationalchip,timer-v1", nc_timer_init);

