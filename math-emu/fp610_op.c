/*
 * linux/arch/csky/math-emu/fp_op.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include "fp_op.h"
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/processor.h>
#include <asm/misc_csky.h>
#include <asm/fpu.h>

#define MASK_3_0_BIT 0xF
#define MASK_8_4_BIT (0x1f << 4)
#define FP_INST_OP_MASK 0xFFF00000

inline unsigned int inst_is_fp(unsigned int inst)
{
	unsigned int result = ((inst & FP_INST_OP_MASK) == 0xFFE00000) ? 1 : 0;
	return result;
}

inline unsigned int get_fp610_inst1(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("cpseti 1\n\r"
				"cprcr %0, cpcr5 \n\r"
				:"=r"(result));

	if (inst_is_fp(result)) {
		return result;
	}

	return 0;
}

inline unsigned int get_fp610_inst2(void)
{
	unsigned int result = 0;
	__asm__ __volatile__("cpseti 1\n\r"
				"cprcr %0, cpcr6 \n\r"
				:"=r"(result));

	if (inst_is_fp(result)) {
		return result;
	}

	return 0;
}

inline unsigned int
get_cpwir_instruction(unsigned int inst, struct pt_regs *regs)
{
	return read_gr((inst & MASK_3_0_BIT), regs);
}

inline void emulate_cprc_inst(unsigned int inst, struct pt_regs *regs)
{
	unsigned int val = get_fsr_c(regs);
	if (val & 0x1) {
		regs->sr |= 0x1;
	} else {
		regs->sr &= ~0x1;
	}
}

inline void emulate_cprgr_inst(unsigned int inst, struct pt_regs *regs)
{
	unsigned int result;
	result = (unsigned int)read_fpr((inst & MASK_8_4_BIT) >> 4);
	write_gr(result, (inst & MASK_3_0_BIT), regs);
}

inline void emulate_cpwgr_inst(unsigned int inst, struct pt_regs *regs)
{
	unsigned int result;
	result = read_gr((inst & MASK_3_0_BIT), regs);
	write_fpr((float32)result, (inst & MASK_8_4_BIT) >> 4);
}

inline unsigned int read_gr(int reg_num, struct pt_regs *regs)
{
	return read_pt_regs(reg_num, regs);
}

inline void write_gr(unsigned int val, int reg_num, struct pt_regs *regs)
{
	write_pt_regs(val, reg_num, regs);
}

inline flag get_fsr_c(struct pt_regs *regs)
{
	return read_fpsr() & 0x1;
}

inline void set_fsr_c(unsigned int val, struct pt_regs *regs)
{
	write_fpsr(0x1 & val);
}

#ifdef CONFIG_CPU_HAS_FPU
inline float32 read_fpr(int reg_num)
{
	float32 result = 0;
	switch (reg_num) {
	case 0x0:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr0 \n\r"
					:"=r"(result));
		break;
	case 0x1:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr1 \n\r"
					:"=r"(result));
		break;
	case 0x2:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr2 \n\r"
					:"=r"(result));
		break;
	case 0x3:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr3 \n\r"
					:"=r"(result));
		break;
	case 0x4:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr4 \n\r"
					:"=r"(result));
		break;
	case 0x5:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr5 \n\r"
					:"=r"(result));
		break;
	case 0x6:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr6 \n\r"
					:"=r"(result));
		break;
	case 0x7:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr7 \n\r"
					:"=r"(result));
		break;
	case 0x8:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr8 \n\r"
					:"=r"(result));
		break;
	case 0x9:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr9 \n\r"
					:"=r"(result));
		break;
	case 0xa:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr10 \n\r"
					:"=r"(result));
		break;
	case 0xb:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr11 \n\r"
					:"=r"(result));
		break;
	case 0xc:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr12 \n\r"
					:"=r"(result));
		break;
	case 0xd:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr13 \n\r"
					:"=r"(result));
		break;
	case 0xe:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr14 \n\r"
					:"=r"(result));
		break;
	case 0xf:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr15 \n\r"
					:"=r"(result));
		break;
	case 0x10:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr16 \n\r"
					:"=r"(result));
		break;
	case 0x11:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr17 \n\r"
					:"=r"(result));
		break;
	case 0x12:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr18 \n\r"
					:"=r"(result));
		break;
	case 0x13:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr19 \n\r"
					:"=r"(result));
		break;
	case 0x14:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr20 \n\r"
					:"=r"(result));
		break;
	case 0x15:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr21 \n\r"
					:"=r"(result));
		break;
	case 0x16:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr22 \n\r"
					:"=r"(result));
		break;
	case 0x17:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr23 \n\r"
					:"=r"(result));
		break;
	case 0x18:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr24 \n\r"
					:"=r"(result));
		break;
	case 0x19:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr25 \n\r"
					:"=r"(result));
		break;
	case 0x1a:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr26 \n\r"
					:"=r"(result));
		break;
	case 0x1b:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr27 \n\r"
					:"=r"(result));
		break;
	case 0x1c:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr28 \n\r"
					:"=r"(result));
		break;
	case 0x1d:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr29 \n\r"
					:"=r"(result));
		break;
	case 0x1e:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr30 \n\r"
					:"=r"(result));
		break;
	case 0x1f:
		__asm__ __volatile__("cpseti 1\n\r"
					"cprgr %0, cpr31 \n\r"
					:"=r"(result));
		break;
	}

	return result;
}

inline void write_fpr(float32 val, int reg_num)
{
	switch (reg_num) {
	case 0x0:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr0 \n\r"
					::"r"(val));
		break;
	case 0x1:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr1 \n\r"
					::"r"(val));
		break;
	case 0x2:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr2 \n\r"
					::"r"(val));
		break;
	case 0x3:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr3 \n\r"
					::"r"(val));
		break;
	case 0x4:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr4 \n\r"
					::"r"(val));
		break;
	case 0x5:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr5 \n\r"
					::"r"(val));
		break;
	case 0x6:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr6 \n\r"
					::"r"(val));
		break;
	case 0x7:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr7 \n\r"
					::"r"(val));
		break;
	case 0x8:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr8 \n\r"
					::"r"(val));
		break;
	case 0x9:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr9 \n\r"
					::"r"(val));
		break;
	case 0xa:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr10 \n\r"
					::"r"(val));
		break;
	case 0xb:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr11 \n\r"
					::"r"(val));
		break;
	case 0xc:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr12 \n\r"
					::"r"(val));
		break;
	case 0xd:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr13 \n\r"
					::"r"(val));
		break;
	case 0xe:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr14 \n\r"
					::"r"(val));
		break;
	case 0xf:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr15 \n\r"
					::"r"(val));
		break;
	case 0x10:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr16 \n\r"
					::"r"(val));
		break;
	case 0x11:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr17 \n\r"
					::"r"(val));
		break;
	case 0x12:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr18 \n\r"
					::"r"(val));
		break;
	case 0x13:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr19 \n\r"
					::"r"(val));
		break;
	case 0x14:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr20 \n\r"
					::"r"(val));
		break;
	case 0x15:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr21 \n\r"
					::"r"(val));
		break;
	case 0x16:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr22 \n\r"
					::"r"(val));
		break;
	case 0x17:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr23 \n\r"
					::"r"(val));
		break;
	case 0x18:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr24 \n\r"
					::"r"(val));
		break;
	case 0x19:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr25 \n\r"
					::"r"(val));
		break;
	case 0x1a:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr26 \n\r"
					::"r"(val));
		break;
	case 0x1b:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr27 \n\r"
					::"r"(val));
		break;
	case 0x1c:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr28 \n\r"
					::"r"(val));
		break;
	case 0x1d:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr29 \n\r"
					::"r"(val));
		break;
	case 0x1e:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr30 \n\r"
					::"r"(val));
		break;
	case 0x1f:
		__asm__ __volatile__("cpseti 1\n\r"
					"cpwgr %0, cpr31 \n\r"
					::"r"(val));
		break;
	}
}

#ifdef __BIG_ENDIAN
inline float64 read_fpr64(int reg_num)
{
	union float64_components result;
	result.i[1] = read_fpr(reg_num);
	result.i[0] = read_fpr(reg_num + 1);

	return 	result.f64;
}

inline void write_fpr64(float64 val, int reg_num)
{
	union float64_components result;
	result.f64 = val;
	write_fpr(result.i[1], reg_num);
	write_fpr(result.i[0], reg_num + 1);
}
#else
inline float64 read_fpr64(int reg_num)
{
	union float64_components result;
	result.i[0] = read_fpr(reg_num);
	result.i[1] = read_fpr(reg_num + 1);

	return 	result.f64;
}

inline void write_fpr64(float64 val, int reg_num)
{
	union float64_components result;
	result.f64 = val;
	write_fpr(result.i[0], reg_num);
	write_fpr(result.i[1], reg_num + 1);
}
#endif

inline float32 read_fpr32l(int reg_num)
{
	return read_fpr(reg_num);
}

inline void write_fpr32l(float32 val, int reg_num)
{
	write_fpr(val, reg_num);
}

#else
inline float64 read_fpr64(int reg_num)
{
	unsigned long *ptr = &(current->thread.fp[reg_num]);
	return 	*(float64 *)ptr;
}

inline float32 read_fpr32l(int reg_num)
{
	return current->thread.fp[reg_num];
}

inline void write_fpr64(float64 val, int reg_num)
{
	unsigned long *ptr = &(current->thread.fp[reg_num]);
	*(float64 *)ptr = val;
}

inline void write_fpr32l(float32 val, int reg_num)
{
	current->thread.fp[reg_num] = (unsigned long)val;
}

inline float32 read_fpr(int reg_num)
{
	unsigned long *ptr = &(current->thread.fp[reg_num]);
        return  *(float32 *)ptr;
}

inline void write_fpr(float32 val, int reg_num)
{
	unsigned long *ptr = &(current->thread.fp[reg_num]);
        *(float32 *)ptr = val;
}
#endif
