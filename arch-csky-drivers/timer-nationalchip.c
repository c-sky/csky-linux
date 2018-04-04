// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>

#include <asm/irq.h>
#include <asm/io.h>

#define TIMER_NAME	"nc_timer"

static void __iomem *reg_base;

#define COUNTER_1_STATUS	0x00
#define COUNTER_1_VALUE		0x04
#define COUNTER_1_CONTROL	0x10
#define COUNTER_1_CONFIG	0x20
#define COUNTER_1_PRE		0x24
#define COUNTER_1_INI		0x28
#define COUNTER_2_STATUS	0x40
#define COUNTER_2_VALUE		0x44
#define COUNTER_2_CONTROL	0x50
#define COUNTER_2_CONFIG	0x60
#define COUNTER_2_PRE		0x64
#define COUNTER_2_INI		0x68
#define COUNTER_3_STATUS	0x80
#define COUNTER_3_VALUE		0x84
#define COUNTER_3_CONTROL	0x90
#define COUNTER_3_CONFIG	0xa0
#define COUNTER_3_PRE		0xa4
#define COUNTER_3_INI		0xa8

static inline void timer_reset(void)
{
	writel_relaxed(0x1, reg_base + COUNTER_1_CONTROL);
	writel_relaxed(0x0, reg_base + COUNTER_1_CONTROL);
	writel_relaxed(0x3, reg_base + COUNTER_1_CONFIG);
	writel_relaxed(26,  reg_base + COUNTER_1_PRE);
}

static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *dev = (struct clock_event_device *) dev_id;

	writel_relaxed(1, reg_base + COUNTER_1_STATUS);

	dev->event_handler(dev);

	return IRQ_HANDLED;
}

static int nc_timer_set_periodic(struct clock_event_device *dev)
{
	timer_reset();

	writel_relaxed(0xFFFFD8EF,	reg_base + COUNTER_1_INI);
	writel_relaxed(0x2,		reg_base + COUNTER_1_CONTROL);

	return 0;
}

static int nc_timer_set_next_event(unsigned long delta, struct clock_event_device *evt)
{
	writel_relaxed(0x1,			reg_base + COUNTER_1_CONTROL);
	writel_relaxed(ULONG_MAX - delta,	reg_base + COUNTER_1_INI);
	writel_relaxed(0x2,			reg_base + COUNTER_1_CONTROL);
	return 0;
}

static int nc_timer_shutdown(struct clock_event_device *dev)
{
	writel_relaxed(0x0, reg_base + COUNTER_1_CONTROL);
	writel_relaxed(0x0, reg_base + COUNTER_1_CONFIG);

	return 0;
}

static struct clock_event_device nc_ced = {
	.name			= TIMER_NAME,
	.features		= CLOCK_EVT_FEAT_PERIODIC
				| CLOCK_EVT_FEAT_ONESHOT,
	.rating			= 200,
	.set_state_shutdown	= nc_timer_shutdown,
	.set_state_periodic	= nc_timer_set_periodic,
	.set_next_event		= nc_timer_set_next_event,

};

static u64 notrace nc_sched_clock_read(void)
{
	return (u64) readl_relaxed(reg_base + COUNTER_2_VALUE);
}

static void nc_csd_enable(void)
{
	writel_relaxed(0x1,	reg_base + COUNTER_2_CONTROL);
	writel_relaxed(0x0,	reg_base + COUNTER_2_CONTROL);
	writel_relaxed(0x1,	reg_base + COUNTER_2_CONFIG);

	writel_relaxed(26,	reg_base + COUNTER_2_PRE);
	writel_relaxed(0,	reg_base + COUNTER_2_INI);
	writel_relaxed(0x2,	reg_base + COUNTER_2_CONTROL);
}

static int __init nc_timer_init(struct device_node *np)
{
	unsigned int irq;
	unsigned int freq;

	/* parse from devicetree */
	reg_base = of_iomap(np, 0);
	if (!reg_base)
		panic("%s, of_iomap err.\n", __func__);

	irq = irq_of_parse_and_map(np, 0);
	if (!irq)
		panic("%s, irq_parse err.\n", __func__);

	if (of_property_read_u32(np, "clock-frequency", &freq))
		panic("%s, clock-frequency error.\n", __func__);

	pr_info("Nationalchip Timer Init, reg: %p, irq: %d, freq: %d.\n",
		reg_base, irq, freq);

	/* setup irq */
	if (request_irq(irq, timer_interrupt, IRQF_TIMER, np->name, &nc_ced))
		panic("%s timer_interrupt error.\n", __func__);

	/* register */
	clockevents_config_and_register(&nc_ced, freq, 1, ULONG_MAX);

	nc_csd_enable();
	clocksource_mmio_init(reg_base + COUNTER_2_VALUE, "nationalchip", freq, 200, 32,
			      clocksource_mmio_readl_up);

	sched_clock_register(nc_sched_clock_read, 32, freq);

	return 0;
}
CLOCKSOURCE_OF_DECLARE(nc_timer, "nationalchip,timer-v1", nc_timer_init);

