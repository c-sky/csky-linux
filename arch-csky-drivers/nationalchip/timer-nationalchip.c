// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.

#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched_clock.h>

#include "timer-of.h"

#define TIMER_NAME	"natchip_timer"
#define TIMER_FREQ	1000000
#define CLKSRC_OFFSET	0x40

#define TIMER_STATUS	0x00
#define TIMER_VALUE	0x04
#define TIMER_CONTRL	0x10
#define TIMER_CONFIG	0x20
#define TIMER_DIV	0x24
#define TIMER_INI	0x28

#define STATUS_CLR	BIT(0)

#define CONTRL_RST	BIT(0)
#define CONTRL_START	BIT(1)

#define CONFIG_EN	BIT(0)
#define CONFIG_IRQ_EN	BIT(1)

static irqreturn_t natchip_timer_interrupt(int irq, void *dev)
{
	struct clock_event_device *ce = (struct clock_event_device *) dev;
	void __iomem *base = timer_of_base(to_timer_of(ce));

	writel_relaxed(STATUS_CLR, base + TIMER_STATUS);

	ce->event_handler(ce);

	return IRQ_HANDLED;
}

static int natchip_timer_set_periodic(struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	/* reset */
	writel_relaxed(CONTRL_RST, base + TIMER_CONTRL);

	/* config the timeout value */
	writel_relaxed(ULONG_MAX - timer_of_period(to_timer_of(ce)),
		       base + TIMER_INI);

	/* enable with irq and start */
	writel_relaxed(CONFIG_EN | CONFIG_IRQ_EN, base + TIMER_CONFIG);
	writel_relaxed(CONTRL_START, base + TIMER_CONTRL);

	return 0;
}

static int natchip_timer_set_oneshot(struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	/* reset */
	writel_relaxed(CONTRL_RST, base + TIMER_CONTRL);

	/* enable with irq and start */
	writel_relaxed(CONFIG_EN | CONFIG_IRQ_EN, base + TIMER_CONFIG);

	return 0;
}

static int natchip_timer_set_next_event(unsigned long delta, struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	/* use reset to pause timer */
	writel_relaxed(CONTRL_RST, base + TIMER_CONTRL);

	/* config next timeout value */
	writel_relaxed(ULONG_MAX - delta, base + TIMER_INI);
	writel_relaxed(CONTRL_START, base + TIMER_CONTRL);

	return 0;
}

static int natchip_timer_shutdown(struct clock_event_device *ce)
{
	void __iomem *base = timer_of_base(to_timer_of(ce));

	writel_relaxed(0, base + TIMER_CONTRL);
	writel_relaxed(0, base + TIMER_CONFIG);

	return 0;
}

static struct timer_of to = {
	.flags = TIMER_OF_IRQ | TIMER_OF_BASE | TIMER_OF_CLOCK,
	.clkevt = {
		.rating			= 300,
		.features		= CLOCK_EVT_FEAT_DYNIRQ |
					  CLOCK_EVT_FEAT_PERIODIC |
					  CLOCK_EVT_FEAT_ONESHOT,
		.set_state_shutdown	= natchip_timer_shutdown,
		.set_state_periodic	= natchip_timer_set_periodic,
		.set_state_oneshot	= natchip_timer_set_oneshot,
		.set_next_event		= natchip_timer_set_next_event,
		.cpumask		= cpu_possible_mask,
	},
	.of_irq = {
		.handler		= natchip_timer_interrupt,
		.flags			= IRQF_TIMER | IRQF_IRQPOLL,
	},
};

static u64 notrace natchip_sched_clock_read(void)
{
	void __iomem *base;

	base = timer_of_base(&to) + CLKSRC_OFFSET; 

	return (u64) readl_relaxed(base + TIMER_VALUE);
}

static void natchip_timer_set_div(void __iomem *base)
{
	unsigned int div;

	div = (timer_of_rate(&to) / TIMER_FREQ) - 1;

	writel_relaxed(div, base + TIMER_DIV);
}

static void natchip_clkevt_init(void __iomem *base)
{
	/* reset */
	writel_relaxed(CONTRL_RST, base + TIMER_CONTRL);

	/* reset config */
	writel_relaxed(0, base + TIMER_CONFIG);

	natchip_timer_set_div(base);

	clockevents_config_and_register(&to.clkevt, TIMER_FREQ, 1, ULONG_MAX);
}

static int natchip_clksrc_init(void __iomem *base)
{
	writel_relaxed(CONTRL_RST, base + TIMER_CONTRL);

	writel_relaxed(CONFIG_EN, base + TIMER_CONFIG);

	natchip_timer_set_div(base);

	writel_relaxed(0, base + TIMER_INI);
	writel_relaxed(CONTRL_START, base + TIMER_CONTRL);

	sched_clock_register(natchip_sched_clock_read, 32, TIMER_FREQ);

	return clocksource_mmio_init(base + TIMER_VALUE, "natchip", TIMER_FREQ, 200, 32,
				     clocksource_mmio_readl_up);

}

static int __init natchip_timer_init(struct device_node *np)
{
	int ret;

	ret = timer_of_init(np, &to);
	if (ret)
		return ret;

	natchip_clkevt_init(timer_of_base(&to));

	return natchip_clksrc_init(timer_of_base(&to) + CLKSRC_OFFSET);
}
TIMER_OF_DECLARE(natchip_timer_gx66xx, "natchip,timer-gx66xx", natchip_timer_init);

