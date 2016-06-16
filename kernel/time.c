#include <linux/sched.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/clocksource.h>

void csky_tick(void)
{
	xtime_update(1);
	update_process_times(user_mode(get_irq_regs()));

	profile_tick(CPU_PROFILING);
}

void __init time_init(void)
{
	of_clk_init(NULL);
	clocksource_probe();
}

