// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/profile.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <linux/sizes.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sched_clock.h>

#define NC_VA_COUNTER_1_STATUS		(void *)(timer_reg + 0x00)
#define NC_VA_COUNTER_1_VALUE		(void *)(timer_reg + 0x04)
#define NC_VA_COUNTER_1_CONTROL		(void *)(timer_reg + 0x10)
#define NC_VA_COUNTER_1_CONFIG		(void *)(timer_reg + 0x20)
#define NC_VA_COUNTER_1_PRE		(void *)(timer_reg + 0x24)
#define NC_VA_COUNTER_1_INI		(void *)(timer_reg + 0x28)
#define NC_VA_COUNTER_2_STATUS		(void *)(timer_reg + 0x40)
#define NC_VA_COUNTER_2_VALUE		(void *)(timer_reg + 0x44)
#define NC_VA_COUNTER_2_CONTROL		(void *)(timer_reg + 0x50)
#define NC_VA_COUNTER_2_CONFIG		(void *)(timer_reg + 0x60)
#define NC_VA_COUNTER_2_PRE		(void *)(timer_reg + 0x64)
#define NC_VA_COUNTER_2_INI		(void *)(timer_reg + 0x68)
#define NC_VA_COUNTER_3_STATUS		(void *)(timer_reg + 0x80)
#define NC_VA_COUNTER_3_VALUE		(void *)(timer_reg + 0x84)
#define NC_VA_COUNTER_3_CONTROL		(void *)(timer_reg + 0x90)
#define NC_VA_COUNTER_3_CONFIG		(void *)(timer_reg + 0xa0)
#define NC_VA_COUNTER_3_PRE		(void *)(timer_reg + 0xa4)
#define NC_VA_COUNTER_3_INI		(void *)(timer_reg + 0xa8)

static unsigned int timer_reg;

static inline void timer_reset(void)
{
	__raw_writel(0x1,	NC_VA_COUNTER_1_CONTROL);
	__raw_writel(0x0,	NC_VA_COUNTER_1_CONTROL);
	__raw_writel(0x3,	NC_VA_COUNTER_1_CONFIG);
	__raw_writel(26,	NC_VA_COUNTER_1_PRE);
}

static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *dev = (struct clock_event_device *) dev_id;

	__raw_writel(1, NC_VA_COUNTER_1_STATUS);

	dev->event_handler(dev);

	return IRQ_HANDLED;
}

static int nc_timer_set_periodic(struct clock_event_device *dev)
{
	timer_reset();

	__raw_writel(0xFFFFD8EF, NC_VA_COUNTER_1_INI);
	__raw_writel(0x2, NC_VA_COUNTER_1_CONTROL);

	return 0;
}

static int nc_timer_set_next_event(unsigned long delta, struct clock_event_device *evt)
{
	__raw_writel(0x1,		NC_VA_COUNTER_1_CONTROL);
	__raw_writel(ULONG_MAX - delta, NC_VA_COUNTER_1_INI);
	__raw_writel(0x2,		NC_VA_COUNTER_1_CONTROL);
	return 0;
}

static int nc_timer_shutdown(struct clock_event_device *dev)
{
	__raw_writel(0x0, NC_VA_COUNTER_1_CONTROL);
	__raw_writel(0x0, NC_VA_COUNTER_1_CONFIG);

	return 0;
}

static struct clock_event_device nc_ced = {
	.name			= "nationalchip-clkevent",
	.features		= CLOCK_EVT_FEAT_PERIODIC
				| CLOCK_EVT_FEAT_ONESHOT,
	.rating			= 200,
	.set_state_shutdown	= nc_timer_shutdown,
	.set_state_periodic	= nc_timer_set_periodic,
	.set_next_event		= nc_timer_set_next_event,
};

static u64 notrace nc_sched_clock_read(void)
{
	return (u64) __raw_readl(NC_VA_COUNTER_2_VALUE);
}

static void nc_csd_enable(void)
{
	__raw_writel(0x1, NC_VA_COUNTER_2_CONTROL);
	__raw_writel(0x0, NC_VA_COUNTER_2_CONTROL);
	__raw_writel(0x1, NC_VA_COUNTER_2_CONFIG);

	__raw_writel(26,NC_VA_COUNTER_2_PRE);
	__raw_writel(0, NC_VA_COUNTER_2_INI);
	__raw_writel(0x2, NC_VA_COUNTER_2_CONTROL);
}

static int __init nc_timer_init(struct device_node *np)
{
	unsigned int irq;
	unsigned int freq;

	/* parse from devicetree */
	timer_reg = (unsigned int) of_iomap(np, 0);
	if (!timer_reg)
		panic("%s, of_iomap err.\n", __func__);

	irq = irq_of_parse_and_map(np, 0);
	if (!irq)
		panic("%s, irq_parse err.\n", __func__);

	if (of_property_read_u32(np, "clock-frequency", &freq))
		panic("%s, clock-frequency error.\n", __func__);

	pr_info("Nationalchip Timer Init, reg: %x, irq: %d, freq: %d.\n",
		timer_reg, irq, freq);

	/* setup irq */
	if (request_irq(irq, timer_interrupt, IRQF_TIMER, np->name, &nc_ced))
		panic("%s timer_interrupt error.\n", __func__);

	/* register */
	clockevents_config_and_register(&nc_ced, freq, 1, ULONG_MAX);

	nc_csd_enable();
	clocksource_mmio_init(NC_VA_COUNTER_2_VALUE, "nationalchip-clksource", freq, 200, 32, clocksource_mmio_readl_up);

	sched_clock_register(nc_sched_clock_read, 32, freq);

	return 0;
}
CLOCKSOURCE_OF_DECLARE(nc_timer, "nationalchip,timer-v1", nc_timer_init);

