// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/clocksource.h>
#include <linux/of_clk.h>
#include <linux/delay.h>

unsigned long csky_timebase __ro_after_init;
EXPORT_SYMBOL_GPL(csky_timebase);

void __init time_init(void)
{
	struct device_node *cpu;
	u32 prop;

	cpu = of_find_node_by_path("/cpus");
	if (!cpu || of_property_read_u32(cpu, "timebase-frequency", &prop))
		goto out;

	of_node_put(cpu);
	csky_timebase = prop;

	lpj_fine = csky_timebase / HZ;
out:
	of_clk_init(NULL);
	timer_probe();
}
