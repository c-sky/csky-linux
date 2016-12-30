#include <linux/clk-provider.h>
#include <linux/clocksource.h>

void __init time_init(void)
{
	of_clk_init(NULL);
	clocksource_probe();
}

