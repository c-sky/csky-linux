/*
 * linux/arch/csky/lib/misc_abiv2.c
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

	if(rx < 14){
		value  = *((int *)regs + rx + 3);
	}else if(rx == 14){
		if(user_mode(regs)){
			__asm__ __volatile__("mfcr %0, cr<14, 1> \n\r"
						:"=r"(value));
		}else{
			value = sizeof(struct pt_regs) + ((unsigned int)regs);
		}
	}else{
		value = *((int *)regs + rx + 2);
	}

	return value;
}

inline void
write_pt_regs(unsigned int value, unsigned int rx, struct pt_regs *regs)
{
	if(rx < 14){
		*((int *)regs + rx + 3) = value;
	}else if(rx == 14){
		if(user_mode(regs)){
			__asm__ __volatile__("mtcr %0, cr<14, 1> \n\r"
						::"r"(value));
		}else{
			printk("math emulate trying to write sp.\n");
		}
	}else{
		*((int *)regs + rx + 2) = value;
	}
}

#ifdef CONFIG_CPU_HAS_FPU
void __init init_fpu(void)
{
	unsigned long fcr;

	/* save global fcr in arch/csky/hal/v-/src/misc.c */
	os_config_fcr = (IDE_STAT | IXE_STAT | UFE_STAT |
			 OFE_STAT | DZE_STAT | IOE_STAT);

	fcr = os_config_fcr;
	__asm__ __volatile__(
			"mtcr	%0, cr<1, 2> \n\t"
			::"r"(fcr)
			);
}

inline unsigned int read_fpcr(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("mfcr %0, cr<1, 2>\n"
				:"=r"(result));
	return result;
}

inline void write_fpcr(unsigned int val)
{
	unsigned int result = val | os_config_fcr;
	__asm__ __volatile__("mtcr %0, cr<1, 2>\n"
				::"r"(result));
}

inline unsigned int read_fpesr(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("mfcr %0, cr<2, 2>\n"
				:"=r"(result));
	return result;
}

inline void write_fpesr(unsigned int val)
{
	unsigned int result = val;
	__asm__ __volatile__("mtcr %0, cr<2, 2>\n"
				::"r"(result));
}

#else /* CONFIG_CPU_HAS_FPU */
void __init init_fpu(void) {}

inline unsigned int read_fpcr(void)
{
	return current->thread.fcr;
}

inline void write_fpcr(unsigned int val)
{
	val |= os_config_fcr;
	current->thread.fcr = val;
}

inline unsigned int read_fpesr(void)
{
	return current->thread.fesr;
}

inline void write_fpesr(unsigned int val)
{
	current->thread.fesr = val;
}

#endif /* CONFIG_CPU_HAS_FPU */
