#include <linux/sched.h>
#include <linux/profile.h>
#include <asm/irq_regs.h>

void (*mach_time_init) (void) __initdata = NULL;

void csky_tick(void)
{
	xtime_update(1);
	update_process_times(user_mode(get_irq_regs()));

	profile_tick(CPU_PROFILING);
}

void __init time_init(void)
{
	if (mach_time_init)
		mach_time_init();
}

