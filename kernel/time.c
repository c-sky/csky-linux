/*
 * linux/arch/csky/kernel/time.c
 *
 * This file contains the csky time handling details.
 * Most of the stuff is located in the machine specific files.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Hu junshan(junshan_hu@c-sky.com) 
 *
 */

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

static inline int set_rtc_mmss(unsigned long nowtime)
{
	if (mach_set_clock_mmss)
		return mach_set_clock_mmss (nowtime);
	return -1;
}


#ifndef CONFIG_GENERIC_CLOCKEVENTS
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
#ifndef CONFIG_SMP
	update_process_times(user_mode(get_irq_regs()));
#endif
	profile_tick(CPU_PROFILING);

#ifdef CONFIG_HEARTBEAT
	/* use power LED as a heartbeat instead -- much more useful
	   for debugging -- based on the version for PReP by Cort */
	/* acts like an actual heart beat -- ie thump-thump-pause... */
	if (mach_heartbeat) {
		static unsigned cnt = 0, period = 0, dist = 0;
		
		if (cnt == 0 || cnt == dist)
			mach_heartbeat( 1 );
		else if (cnt == 7 || cnt == dist+7)
			mach_heartbeat( 0 );
		
		if (++cnt > period) {
			cnt = 0;
			/* The hyperbolic function below modifies the heartbeat period
			* length in dependency of the current (5min) load. It goes
			* through the points f(0)=126, f(1)=86, f(5)=51,
			* f(inf)->30. */
			period = ((672<<FSHIFT)/(5*avenrun[0]+(7<<FSHIFT))) + 30;
			dist = period / 4;
		}
	}
#endif /* CONFIG_HEARTBEAT */

	return IRQ_HANDLED;
}
#endif

void read_persistent_clock(struct timespec *ts)
{
	struct rtc_time time;
	ts->tv_sec = 0;
	ts->tv_nsec = 0;

	if (mach_hwclk) {
		mach_hwclk(0, &time);

		if ((time.tm_year += 1900) < 1970)
			time.tm_year += 100;
		ts->tv_sec = mktime(time.tm_year, time.tm_mon, time.tm_mday,
				      time.tm_hour, time.tm_min, time.tm_sec);
	}
}

void __init time_init(void)
{
	mach_time_init();
}

#if defined(CONFIG_PM) && !defined(CONFIG_GENERIC_CLOCKEVENTS)
static int timer_suspend(void)
{
	if (mach_time_suspend)
		mach_time_suspend();

	return 0;
}

static void timer_resume(void)
{
	if (mach_time_resume)
		mach_time_resume();
}
#else
#define timer_suspend NULL
#define timer_resume NULL
#endif

static struct syscore_ops timer_syscore_ops = {
	.suspend    = timer_suspend,
	.resume     = timer_resume,
};

static int __init timer_init_syscore_ops(void)
{
	register_syscore_ops(&timer_syscore_ops);
	
	return 0;
}

device_initcall(timer_init_syscore_ops);

#ifdef CONFIG_ARCH_USES_GETTIMEOFFSET
u32 arch_gettimeoffset(void)
{
	return mach_gettimeoffset() * 1000;
}
#endif /* CONFIG_ARCH_USES_GETTIMEOFFSET */
