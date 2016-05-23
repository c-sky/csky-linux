#include <linux/errno.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/param.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>

#include <asm/machdep.h>
#include <asm/io.h>
#include <asm/irq_regs.h>

#include <linux/time.h>
#include <linux/syscore_ops.h>
#include <linux/timex.h>
#include <linux/profile.h>

#define TICK_SIZE (tick_nsec / 1000)

/*
 * timer_interrupt() needs to keep up the real-time clock,
 * as well as call the "do_timer()" routine every clocktick
 */
irqreturn_t  csky_timer_interrupt(int irq, void *dummy)
{
	/* may need to kick the hardware timer */
	if (mach_tick)
		mach_tick();
	xtime_update(1);
	update_process_times(user_mode(get_irq_regs()));

	profile_tick(CPU_PROFILING);

	return IRQ_HANDLED;
}

void __init time_init(void)
{
	mach_time_init();
}

#ifdef CONFIG_ARCH_USES_GETTIMEOFFSET
u32 arch_gettimeoffset(void)
{
	return mach_gettimeoffset() * 1000;
}
#endif /* CONFIG_ARCH_USES_GETTIMEOFFSET */
