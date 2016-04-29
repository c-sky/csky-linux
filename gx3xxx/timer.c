/*
 * arch/csky/gx3xxx/timer.c --- timer handles
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004, Kang Sun (sunk@vlsi.zju.edu.cn)
 * Copyright (C) 2004, Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009, Hu Junshan (junshan_hu@c-sky.com)
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/rtc.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <mach/map.h>
#include <asm/delay.h>
#include <asm/io_mm.h>

extern irqreturn_t csky_timer_interrupt(int irq, void *dummy);
void gx3201_tick(void)
{
	__raw_writel(1,GX3201_VA_COUNTER_1_STATUS);
	
}
static void gx3201_timer_reset(void)
{
	__raw_writel(0x1, GX3201_VA_COUNTER_1_CONTROL);
	__raw_writel(0x0, GX3201_VA_COUNTER_1_CONTROL);
	__raw_writel(0x3, GX3201_VA_COUNTER_1_CONFIG);

	__raw_writel(26,GX3201_VA_COUNTER_1_PRE);
	__raw_writel(0xFFFFD8EF,GX3201_VA_COUNTER_1_INI);
	__raw_writel(0x2, GX3201_VA_COUNTER_1_CONTROL);
}

static struct irqaction gx3201_timer_irq = {
        .name           = "Gx3201 Timer Tick",
        .flags          = IRQF_SHARED | IRQF_TIMER,
        .handler        = csky_timer_interrupt,
};

void __init gx3201_timer_init(irq_handler_t handler)
{
	gx3201_timer_reset();
	setup_irq(10,&gx3201_timer_irq);
}


int gx3201_hwclk(int set, struct rtc_time *t)
{
	t->tm_year = 1980;
	t->tm_mon  = 1;
	t->tm_mday = 1;
	t->tm_hour = 0;
	t->tm_min  = 0;
	t->tm_sec  = 0;	

	return 0;
}

