/*
 * linux/arch/csky/math-emu/fp_op.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */

#include "fp_op.h"
#include <linux/kernel.h>

#if 0
#define P_DEBUG(arg)	printk arg
#else
#define P_DEBUG(arg)	do {} while(0)
#endif

const float64 float64_constant[] = {
	0x0000000000000000ULL,	/* double 0.0 */
	0x3ff0000000000000ULL,	/* double 1.0 */
	0x4000000000000000ULL,	/* double 2.0 */
	0x4008000000000000ULL,	/* double 3.0 */
	0x4010000000000000ULL,	/* double 4.0 */
	0x4014000000000000ULL,	/* double 5.0 */
	0x3fe0000000000000ULL,	/* double 0.5 */
	0x4024000000000000ULL	/* double 10.0 */
};

const float32 float32_constant[] = {
	0x00000000,		/* single 0.0 */
	0x3f800000,		/* single 1.0 */
	0x40000000,		/* single 2.0 */
	0x40400000,		/* single 3.0 */
	0x40800000,		/* single 4.0 */
	0x40a00000,		/* single 5.0 */
	0x3f000000,		/* single 0.5 */
	0x41200000		/* single 10.0 */
};

#define ROUND_MODE_MASK	(0x3 << 24)

inline float64 get_double_constant(const unsigned int index)
{
	return float64_constant[index];
}

inline float32 get_single_constant(const unsigned int index)
{
	return float32_constant[index];
}

inline unsigned int get_round_mode(void)
{
	unsigned int result = read_fpcr();

	return result & ROUND_MODE_MASK;
}

inline void set_round_mode(unsigned int val)
{
	write_fpcr(val & ROUND_MODE_MASK);
}

inline void clear_fesr(unsigned int fesr)
{
	write_fpesr(0);
}


inline float64 get_float64(int reg_num)
{
	float64 result = read_fpr64(reg_num);
	P_DEBUG(("get_float64: result = %lld, reg_num = %d\n", result, reg_num));

	return result;
}

inline float32 get_float32(int reg_num)
{
	float32 result = read_fpr32l(reg_num);
	P_DEBUG(("get_float32: result = %x, reg_num = %d\n", result, reg_num));
	return result;
}

inline void set_float64(float64 val, int reg_num)
{
	P_DEBUG(("set_float64: val = %lld, reg_num = %d\n", val, reg_num));
	write_fpr64(val, reg_num);
}

inline void set_float32(float32 val, int reg_num)
{
	P_DEBUG(("set_float32: val = %x, reg_num = %d\n", val, reg_num));
	write_fpr32l(val, reg_num);
}

#ifndef CONFIG_CPU_CSKYV1
inline float32 get_float32h(int reg_num)
{
	float32 result = read_fpr32h(reg_num);
	P_DEBUG(("get_float32h: result = %x, reg_num = %d\n", result, reg_num));
	return result;
}

inline void set_float32h(float32 val, int reg_num)
{
	P_DEBUG(("set_float32h: val = %x, reg_num = %d\n", val, reg_num));
	write_fpr32h(val, reg_num);
}
#endif

inline unsigned int get_uint32(int reg_num, struct inst_data *inst_data)
{
	unsigned int result = read_gr(reg_num, inst_data->regs);
	P_DEBUG(("set_uint32: result = %x, reg_num = %d\n", result, reg_num));
	return result;
}

inline void set_uint32(unsigned int val, int reg_num, struct inst_data *inst_data)
{
	P_DEBUG(("set_uint32: val = %x, reg_num = %d\n", val, reg_num));
	write_gr(val, reg_num, inst_data->regs);
}

#ifndef CONFIG_CPU_CSKYV1
inline float64 get_float64_from_memory(unsigned long addr)
{
	return get_fpvalue64(addr);
}

inline void set_float64_to_memory(float64 val, unsigned long addr)
{
	set_fpvalue64(val, addr);
}

inline float32 get_float32_from_memory(unsigned long addr)
{
	return get_fpvalue32(addr);
}

inline void set_float32_to_memory(float32 val, unsigned long addr)
{
	set_fpvalue32(val, addr);
}
#endif
