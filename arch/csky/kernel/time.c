#include <linux/clk-provider.h>
#include <linux/clocksource.h>

void __init time_init(void)
{
	of_clk_init(NULL);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)) 
	clocksource_probe();
#else
	timer_probe();
#endif
}

