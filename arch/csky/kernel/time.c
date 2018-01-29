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

