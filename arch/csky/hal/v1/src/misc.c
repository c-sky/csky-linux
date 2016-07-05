/*
 * linux/arch/csky/lib/misc_abiv1.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */


#include <asm/ptrace.h>
#include <linux/kernel.h>
#include <linux/sched.h>

unsigned long os_config_fcr;

inline unsigned int
read_pt_regs(unsigned int rx, struct pt_regs *regs)
{
	unsigned int value;

	if (rx == 0) {
		if (user_mode(regs)) {
			__asm__ __volatile__("mfcr %0, ss1 \n\r"
						:"=r"(value));
		} else {
			value = sizeof(struct pt_regs) + ((unsigned int)regs);
		}
	} else if (rx == 1){
		value = regs->regs[9];
	} else if (rx == 15){
		value = regs->r15;
	} else {
		value = *((int *)regs + rx + 1);
	}

	return value;
}

inline void
write_pt_regs(unsigned int value, unsigned int rx, struct pt_regs *regs)
{
	if (rx == 0) {
		printk("math emulate trying to write sp.\n");
	} else if (rx == 1) {
		regs->regs[9] = value;
	} else if (rx == 15) {
		regs->r15 = value;
	} else {
		*((int *)regs + rx + 1) = value;
	}
}

#ifdef CONFIG_CPU_HAS_FPU
inline unsigned int read_fpsr(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("cpseti 1\n\r"
				"cprcr %0, cpcr2 \n\r"
				:"=r"(result));
	return result;
}

inline void write_fpsr(unsigned int val)
{
	__asm__ __volatile__("cpseti 1\n\r"
				"cpwcr %0, cpcr2 \n\r"
				::"r"(val));
}

inline unsigned int read_fpcr(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("cpseti 1\n\r"
				"cprcr %0, cpcr1 \n\r"
				:"=r"(result));
	return result;
}

inline void write_fpcr(unsigned int val)
{
	val |= os_config_fcr;
	__asm__ __volatile__("cpseti 1\n\r"
				"cpwcr %0, cpcr1 \n\r"
				::"r"(val));
}

inline unsigned int read_fpesr(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("cpseti 1\n\r"
				"cprcr %0, cpcr4 \n\r"
				:"=r"(result));
	return result;
}

inline void write_fpesr(unsigned int val)
{
	__asm__ __volatile__("cpseti 1\n\r"
				"cpwcr %0, cpcr4 \n\r"
				::"r"(val));
}
#else
inline unsigned int read_fpcr(void)
{
	return current->thread.fcr;
}

inline void write_fpcr(unsigned int val)
{
	val |= os_config_fcr;
	current->thread.fcr = val;
}

inline unsigned int read_fpsr(void)
{
	return current->thread.fsr;
}

inline void write_fpsr(unsigned int val)
{
	current->thread.fsr = val;
}

inline unsigned int read_fpesr(void)
{
	return current->thread.fesr;
}

inline void write_fpesr(unsigned int val)
{
	current->thread.fesr = val;
}
#endif
