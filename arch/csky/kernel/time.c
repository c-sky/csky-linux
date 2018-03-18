// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/clk-provider.h>
#include <linux/clocksource.h>

void __init time_init(void)
{
	of_clk_init(NULL);
#ifdef COMPAT_KERNEL_4_9
	clocksource_probe();
#else
	timer_probe();
#endif
}

