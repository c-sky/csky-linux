#include <linux/clocksource.h>

void __init time_init(void)
{
	clocksource_probe();
}

