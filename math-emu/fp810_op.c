/*
 * linux/arch/csky/math-emu/fp_op.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */

#include <linux/uaccess.h>
#include <asm/ptrace.h>
#include <asm/misc_csky.h>
#include <asm/fpu.h>
#include "fp_op.h"


#define FP_INST_OP_MASK 0xFE000000
#define IMM4_MASK (0xf << 21)
#define IMM4L_MASK (0xf << 4)

inline unsigned int inst_is_fp(unsigned int inst)
{
	unsigned int result = ((inst & FP_INST_OP_MASK) == 0xF4000000) ? 1 : 0;
	return result;
}

inline unsigned int get_fp810_instruction(struct pt_regs *regs)
{
	unsigned short inst_low, inst_high;
	unsigned int result, inst;
	unsigned int inst_ptr = instruction_pointer(regs);

	result = 0;
	inst = 0;
	inst_low = 0;
	inst_high = 0;

	if (__get_user(inst_low, (unsigned short *)inst_ptr) == 0) {
		if (__get_user(inst_high, (unsigned short *)(inst_ptr + 2)) == 0) {
			inst = inst_high | ((unsigned int)inst_low << 16);
			if (inst_is_fp(inst)) {
				result = inst;
			}
		}
	}

	return result;
}

inline unsigned int get_imm8(struct inst_data *inst_data)
{
	unsigned int inst = inst_data->inst;
	inst = ((inst & IMM4_MASK) >> 17) | ((inst & IMM4L_MASK) >> 4);

	return inst;
}

/*
 * get bit 6-7 for fldrd/fldrs..
 */
inline unsigned int get_imm4l(struct inst_data *inst_data)
{
	unsigned int inst = inst_data->inst;
	inst = (inst & IMM4L_MASK) >> 5;
	return inst;
}

inline unsigned int read_gr(int reg_num, struct pt_regs *regs)
{
	return read_pt_regs(reg_num, regs);
}

inline void write_gr(unsigned int val, int reg_num, struct pt_regs *regs)
{
	write_pt_regs(val, reg_num, regs);
}

/*
 * 810 does not have fsr regiter, just use C bit.
 */
inline flag get_fsr_c(struct pt_regs *regs)
{
	flag result = regs->sr & 0x1;
	return result;
}

/*
 * 810 does not have fsr regiter, just use C bit.
 */
inline void set_fsr_c(unsigned int val, struct pt_regs *regs)
{
	if (val) {
		regs->sr |= 0x1;
	} else {
		regs->sr &= 0xfffffffe;
	}
}

inline float32 get_fpvalue32(unsigned int addr)
{
	float32 result = 0;
	get_user(result, (float32 *)addr);

	return result;
}

inline void set_fpvalue32(unsigned int val, unsigned int addr)
{
	float32 result = (float32)val;

	put_user(result, (float32 *)addr);
}

inline float64 get_fpvalue64(unsigned int addr)
{
	union float64_components result;
#ifdef __BIG_ENDIAN
	get_user(result.i[1], (float32 *)addr);
	get_user(result.i[0], (float32 *)(addr + 4));
#else
	get_user(result.i[1], (float32 *)(addr + 4));
	get_user(result.i[0], (float32 *)addr);
#endif

	return result.f64;
}

inline void set_fpvalue64(float64 val, unsigned int addr)
{
	union float64_components result;
	result.f64 = val;
#ifdef __BIG_ENDIAN
	put_user(result.i[1], (float32 *)addr);
	put_user(result.i[0], (float32 *)(addr + 4));
#else
	put_user(result.i[1], (float32 *)(addr + 4));
	put_user(result.i[0], (float32 *)addr);
#endif
}

#ifdef CONFIG_CPU_HAS_FPU
#ifdef __BIG_ENDIAN
inline float64 read_fpr64(int reg_num)
{
	union float64_components result;
	switch(reg_num) {
	case 0:
		__asm__ __volatile__("fmfvrh %0, vr0\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr0\n"
					:"+r"(result.i[1]));
		break;
	case 1:
		__asm__ __volatile__("fmfvrh %0, vr1\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr1\n"
					:"+r"(result.i[1]));
		break;
	case 2:
		__asm__ __volatile__("fmfvrh %0, vr2\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr2\n"
					:"+r"(result.i[1]));
		break;
	case 3:
		__asm__ __volatile__("fmfvrh %0, vr3\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr3\n"
					:"+r"(result.i[1]));
		break;
	case 4:
		__asm__ __volatile__("fmfvrh %0, vr4\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr4\n"
					:"+r"(result.i[1]));
		break;
	case 5:
		__asm__ __volatile__("fmfvrh %0, vr5\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr5\n"
					:"+r"(result.i[1]));
		break;
	case 6:
		__asm__ __volatile__("fmfvrh %0, vr6\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr6\n"
					:"+r"(result.i[1]));
		break;
	case 7:
		__asm__ __volatile__("fmfvrh %0, vr7\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr7\n"
					:"+r"(result.i[1]));
		break;
	case 8:
		__asm__ __volatile__("fmfvrh %0, vr8\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr8\n"
					:"+r"(result.i[1]));
		break;
	case 9:
		__asm__ __volatile__("fmfvrh %0, vr9\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr9\n"
					:"+r"(result.i[1]));
		break;
	case 10:
		__asm__ __volatile__("fmfvrh %0, vr10\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr10\n"
					:"+r"(result.i[1]));
		break;
	case 11:
		__asm__ __volatile__("fmfvrh %0, vr11\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr11\n"
					:"+r"(result.i[1]));
		break;
	case 12:
		__asm__ __volatile__("fmfvrh %0, vr12\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr12\n"
					:"+r"(result.i[1]));
		break;
	case 13:
		__asm__ __volatile__("fmfvrh %0, vr13\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr13\n"
					:"+r"(result.i[1]));
		break;
	case 14:
		__asm__ __volatile__("fmfvrh %0, vr14\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr14\n"
					:"+r"(result.i[1]));
		break;
	case 15:
		__asm__ __volatile__("fmfvrh %0, vr15\n"
					:"+r"(result.i[0]));
		__asm__ __volatile__("fmfvrl %0, vr15\n"
					:"+r"(result.i[1]));
	}

	return result.f64;
}

inline void write_fpr64(float64 val, int reg_num)
{
	union float64_components result;
	result.f64 = val;
	switch(reg_num) {
	case 0:
		__asm__ __volatile__("fmtvrh vr0, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr0, %0\n"
					::"r"(result.i[1]));
		break;
	case 1:
		__asm__ __volatile__("fmtvrh vr1, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr1, %0\n"
					::"r"(result.i[1]));
		break;
	case 2:
		__asm__ __volatile__("fmtvrh vr2, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr2, %0\n"
					::"r"(result.i[1]));
		break;
	case 3:
		__asm__ __volatile__("fmtvrh vr3, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr3, %0\n"
					::"r"(result.i[1]));
		break;
	case 4:
		__asm__ __volatile__("fmtvrh vr4, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr4, %0\n"
					::"r"(result.i[1]));
		break;
	case 5:
		__asm__ __volatile__("fmtvrh vr5, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr5, %0\n"
					::"r"(result.i[1]));
		break;
	case 6:
		__asm__ __volatile__("fmtvrh vr6, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr6, %0\n"
					::"r"(result.i[1]));
		break;
	case 7:
		__asm__ __volatile__("fmtvrh vr7, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr7, %0\n"
					::"r"(result.i[1]));
		break;
	case 8:
		__asm__ __volatile__("fmtvrh vr8, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr8, %0\n"
					::"r"(result.i[1]));
		break;
	case 9:
		__asm__ __volatile__("fmtvrh vr9, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr9, %0\n"
					::"r"(result.i[1]));
		break;
	case 10:
		__asm__ __volatile__("fmtvrh vr10, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr10, %0\n"
					::"r"(result.i[1]));
		break;
	case 11:
		__asm__ __volatile__("fmtvrh vr11, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr11, %0\n"
					::"r"(result.i[1]));
		break;
	case 12:
		__asm__ __volatile__("fmtvrh vr12, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr12, %0\n"
					::"r"(result.i[1]));
		break;
	case 13:
		__asm__ __volatile__("fmtvrh vr13, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr13, %0\n"
					::"r"(result.i[1]));
		break;
	case 14:
		__asm__ __volatile__("fmtvrh vr14, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr14, %0\n"
					::"r"(result.i[1]));
		break;
	case 15:
		__asm__ __volatile__("fmtvrh vr15, %0\n"
					::"r"(result.i[0]));
		__asm__ __volatile__("fmtvrl vr15, %0\n"
					::"r"(result.i[1]));
		break;
	}
}
#else /* __BIG_ENDIAN */
inline float64 read_fpr64(int reg_num)
{
	union float64_components result;
	switch(reg_num) {
	case 0:
		__asm__ __volatile__("fmfvrh %0, vr0\n"
					:"=r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr0\n"
					:"+r"(result.i[0]));
		break;
	case 1:
		__asm__ __volatile__("fmfvrh %0, vr1\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr1\n"
					:"+r"(result.i[0]));
		break;
	case 2:
		__asm__ __volatile__("fmfvrh %0, vr2\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr2\n"
					:"+r"(result.i[0]));
		break;
	case 3:
		__asm__ __volatile__("fmfvrh %0, vr3\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr3\n"
					:"+r"(result.i[0]));
		break;
	case 4:
		__asm__ __volatile__("fmfvrh %0, vr4\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr4\n"
					:"+r"(result.i[0]));
		break;
	case 5:
		__asm__ __volatile__("fmfvrh %0, vr5\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr5\n"
					:"+r"(result.i[0]));
		break;
	case 6:
		__asm__ __volatile__("fmfvrh %0, vr6\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr6\n"
					:"+r"(result.i[0]));
		break;
	case 7:
		__asm__ __volatile__("fmfvrh %0, vr7\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr7\n"
					:"+r"(result.i[0]));
		break;
	case 8:
		__asm__ __volatile__("fmfvrh %0, vr8\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr8\n"
					:"+r"(result.i[0]));
		break;
	case 9:
		__asm__ __volatile__("fmfvrh %0, vr9\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr9\n"
					:"+r"(result.i[0]));
		break;
	case 10:
		__asm__ __volatile__("fmfvrh %0, vr10\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr10\n"
					:"+r"(result.i[0]));
		break;
	case 11:
		__asm__ __volatile__("fmfvrh %0, vr11\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr11\n"
					:"+r"(result.i[0]));
		break;
	case 12:
		__asm__ __volatile__("fmfvrh %0, vr12\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr12\n"
					:"+r"(result.i[0]));
		break;
	case 13:
		__asm__ __volatile__("fmfvrh %0, vr13\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr13\n"
					:"+r"(result.i[0]));
		break;
	case 14:
		__asm__ __volatile__("fmfvrh %0, vr14\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr14\n"
					:"+r"(result.i[0]));
		break;
	case 15:
		__asm__ __volatile__("fmfvrh %0, vr15\n"
					:"+r"(result.i[1]));
		__asm__ __volatile__("fmfvrl %0, vr15\n"
					:"+r"(result.i[0]));
	}

	return result.f64;
}

inline void write_fpr64(float64 val, int reg_num)
{
	union float64_components result;
	result.f64 = val;
	switch(reg_num) {
	case 0:
		__asm__ __volatile__("fmtvrh vr0, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr0, %0\n"
					::"r"(result.i[0]));
		break;
	case 1:
		__asm__ __volatile__("fmtvrh vr1, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr1, %0\n"
					::"r"(result.i[0]));
		break;
	case 2:
		__asm__ __volatile__("fmtvrh vr2, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr2, %0\n"
					::"r"(result.i[0]));
		break;
	case 3:
		__asm__ __volatile__("fmtvrh vr3, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr3, %0\n"
					::"r"(result.i[0]));
		break;
	case 4:
		__asm__ __volatile__("fmtvrh vr4, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr4, %0\n"
					::"r"(result.i[0]));
		break;
	case 5:
		__asm__ __volatile__("fmtvrh vr5, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr5, %0\n"
					::"r"(result.i[0]));
		break;
	case 6:
		__asm__ __volatile__("fmtvrh vr6, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr6, %0\n"
					::"r"(result.i[0]));
		break;
	case 7:
		__asm__ __volatile__("fmtvrh vr7, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr7, %0\n"
					::"r"(result.i[0]));
		break;
	case 8:
		__asm__ __volatile__("fmtvrh vr8, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr8, %0\n"
					::"r"(result.i[0]));
		break;
	case 9:
		__asm__ __volatile__("fmtvrh vr9, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr9, %0\n"
					::"r"(result.i[0]));
		break;
	case 10:
		__asm__ __volatile__("fmtvrh vr10, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr10, %0\n"
					::"r"(result.i[0]));
		break;
	case 11:
		__asm__ __volatile__("fmtvrh vr11, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr11, %0\n"
					::"r"(result.i[0]));
		break;
	case 12:
		__asm__ __volatile__("fmtvrh vr12, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr12, %0\n"
					::"r"(result.i[0]));
		break;
	case 13:
		__asm__ __volatile__("fmtvrh vr13, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr13, %0\n"
					::"r"(result.i[0]));
		break;
	case 14:
		__asm__ __volatile__("fmtvrh vr14, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr14, %0\n"
					::"r"(result.i[0]));
		break;
	case 15:
		__asm__ __volatile__("fmtvrh vr15, %0\n"
					::"r"(result.i[1]));
		__asm__ __volatile__("fmtvrl vr15, %0\n"
					::"r"(result.i[0]));
		break;
	}
}
#endif /* __BIG_ENDIAN */

inline float32 read_fpr32l(int reg_num)
{
	float32 result = 0;
	switch(reg_num) {
	case 0:
		__asm__ __volatile__("fmfvrl %0, vr0\n"
					:"+r"(result));
		break;
	case 1:
		__asm__ __volatile__("fmfvrl %0, vr1\n"
					:"+r"(result));
		break;
	case 2:
		__asm__ __volatile__("fmfvrl %0, vr2\n"
					:"+r"(result));
		break;
	case 3:
		__asm__ __volatile__("fmfvrl %0, vr3\n"
					:"+r"(result));
		break;
	case 4:
		__asm__ __volatile__("fmfvrl %0, vr4\n"
					:"+r"(result));
		break;
	case 5:
		__asm__ __volatile__("fmfvrl %0, vr5\n"
					:"+r"(result));
		break;
	case 6:
		__asm__ __volatile__("fmfvrl %0, vr6\n"
					:"+r"(result));
		break;
	case 7:
		__asm__ __volatile__("fmfvrl %0, vr7\n"
					:"+r"(result));
		break;
	case 8:
		__asm__ __volatile__("fmfvrl %0, vr8\n"
					:"+r"(result));
		break;
	case 9:
		__asm__ __volatile__("fmfvrl %0, vr9\n"
					:"+r"(result));
		break;
	case 10:
		__asm__ __volatile__("fmfvrl %0, vr10\n"
					:"+r"(result));
		break;
	case 11:
		__asm__ __volatile__("fmfvrl %0, vr11\n"
					:"+r"(result));
		break;
	case 12:
		__asm__ __volatile__("fmfvrl %0, vr12\n"
					:"+r"(result));
		break;
	case 13:
		__asm__ __volatile__("fmfvrl %0, vr13\n"
					:"+r"(result));
		break;
	case 14:
		__asm__ __volatile__("fmfvrl %0, vr14\n"
					:"+r"(result));
		break;
	case 15:
		__asm__ __volatile__("fmfvrl %0, vr15\n"
					:"+r"(result));
	}

	return result;
}

inline void write_fpr32l(float32 val, int reg_num)
{
	float32 result = val;
	switch (reg_num) {
	case 0:
		__asm__ __volatile__("fmtvrl vr0, %0\n"
					::"r"(result));
		break;
	case 1:
		__asm__ __volatile__("fmtvrl vr1, %0\n"
					::"r"(result));
		break;
	case 2:
		__asm__ __volatile__("fmtvrl vr2, %0\n"
					::"r"(result));
		break;
	case 3:
		__asm__ __volatile__("fmtvrl vr3, %0\n"
					::"r"(result));
		break;
	case 4:
		__asm__ __volatile__("fmtvrl vr4, %0\n"
					::"r"(result));
		break;
	case 5:
		__asm__ __volatile__("fmtvrl vr5, %0\n"
					::"r"(result));
		break;
	case 6:
		__asm__ __volatile__("fmtvrl vr6, %0\n"
					::"r"(result));
		break;
	case 7:
		__asm__ __volatile__("fmtvrl vr7, %0\n"
					::"r"(result));
		break;
	case 8:
		__asm__ __volatile__("fmtvrl vr8, %0\n"
					::"r"(result));
		break;
	case 9:
		__asm__ __volatile__("fmtvrl vr9, %0\n"
					::"r"(result));
		break;
	case 10:
		__asm__ __volatile__("fmtvrl vr10, %0\n"
					::"r"(result));
		break;
	case 11:
		__asm__ __volatile__("fmtvrl vr11, %0\n"
					::"r"(result));
		break;
	case 12:
		__asm__ __volatile__("fmtvrl vr12, %0\n"
					::"r"(result));
		break;
	case 13:
		__asm__ __volatile__("fmtvrl vr13, %0\n"
					::"r"(result));
		break;
	case 14:
		__asm__ __volatile__("fmtvrl vr14, %0\n"
					::"r"(result));
		break;
	case 15:
		__asm__ __volatile__("fmtvrl vr15, %0\n"
					::"r"(result));
		break;
	}
}

inline float32 read_fpr32h(int reg_num)
{
	float32 result = 0;
	switch(reg_num) {
	case 0:
		__asm__ __volatile__("fmfvrh %0, vr0\n"
					:"+r"(result));
		break;
	case 1:
		__asm__ __volatile__("fmfvrh %0, vr1\n"
					:"+r"(result));
		break;
	case 2:
		__asm__ __volatile__("fmfvrh %0, vr2\n"
					:"+r"(result));
		break;
	case 3:
		__asm__ __volatile__("fmfvrh %0, vr3\n"
					:"+r"(result));
		break;
	case 4:
		__asm__ __volatile__("fmfvrh %0, vr4\n"
					:"+r"(result));
		break;
	case 5:
		__asm__ __volatile__("fmfvrh %0, vr5\n"
					:"+r"(result));
		break;
	case 6:
		__asm__ __volatile__("fmfvrh %0, vr6\n"
					:"+r"(result));
		break;
	case 7:
		__asm__ __volatile__("fmfvrh %0, vr7\n"
					:"+r"(result));
		break;
	case 8:
		__asm__ __volatile__("fmfvrh %0, vr8\n"
					:"+r"(result));
		break;
	case 9:
		__asm__ __volatile__("fmfvrh %0, vr9\n"
					:"+r"(result));
		break;
	case 10:
		__asm__ __volatile__("fmfvrh %0, vr10\n"
					:"+r"(result));
		break;
	case 11:
		__asm__ __volatile__("fmfvrh %0, vr11\n"
					:"+r"(result));
		break;
	case 12:
		__asm__ __volatile__("fmfvrh %0, vr12\n"
					:"+r"(result));
		break;
	case 13:
		__asm__ __volatile__("fmfvrh %0, vr13\n"
					:"+r"(result));
		break;
	case 14:
		__asm__ __volatile__("fmfvrh %0, vr14\n"
					:"+r"(result));
		break;
	case 15:
		__asm__ __volatile__("fmfvrh %0, vr15\n"
					:"+r"(result));
	}

	return result;
}

inline void write_fpr32h(float32 val, int reg_num)
{
	float32 result = val;
	switch (reg_num) {
	case 0:
		__asm__ __volatile__("fmtvrh vr0, %0\n"
					::"r"(result));
		break;
	case 1:
		__asm__ __volatile__("fmtvrh vr1, %0\n"
					::"r"(result));
		break;
	case 2:
		__asm__ __volatile__("fmtvrh vr2, %0\n"
					::"r"(result));
		break;
	case 3:
		__asm__ __volatile__("fmtvrh vr3, %0\n"
					::"r"(result));
		break;
	case 4:
		__asm__ __volatile__("fmtvrh vr4, %0\n"
					::"r"(result));
		break;
	case 5:
		__asm__ __volatile__("fmtvrh vr5, %0\n"
					::"r"(result));
		break;
	case 6:
		__asm__ __volatile__("fmtvrh vr6, %0\n"
					::"r"(result));
		break;
	case 7:
		__asm__ __volatile__("fmtvrh vr7, %0\n"
					::"r"(result));
		break;
	case 8:
		__asm__ __volatile__("fmtvrh vr8, %0\n"
					::"r"(result));
		break;
	case 9:
		__asm__ __volatile__("fmtvrh vr9, %0\n"
					::"r"(result));
		break;
	case 10:
		__asm__ __volatile__("fmtvrh vr10, %0\n"
					::"r"(result));
		break;
	case 11:
		__asm__ __volatile__("fmtvrh vr11, %0\n"
					::"r"(result));
		break;
	case 12:
		__asm__ __volatile__("fmtvrh vr12, %0\n"
					::"r"(result));
		break;
	case 13:
		__asm__ __volatile__("fmtvrh vr13, %0\n"
					::"r"(result));
		break;
	case 14:
		__asm__ __volatile__("fmtvrh vr14, %0\n"
					::"r"(result));
		break;
	case 15:
		__asm__ __volatile__("fmtvrh vr15, %0\n"
					::"r"(result));
		break;
	}
}

#else /* CONFIG_CPU_HAS_FPU */
/* the slot of reg_num * 2 is always the low 32 bits. */
#ifdef __BIG_ENDIAN
inline float64 read_fpr64(int reg_num)
{
	union float64_components result;
	result.i[1] = current->thread.fp[reg_num * 2];
	result.i[0] = current->thread.fp[reg_num * 2 + 1];
	return 	result.f64;
}

inline void write_fpr64(float64 val, int reg_num)
{
	union float64_components result;
	result.f64 = val;
	current->thread.fp[reg_num * 2] = result.i[1];
	current->thread.fp[reg_num * 2 + 1] = result.i[0];
}
#else /* __BIG_ENDIAN */
inline float64 read_fpr64(int reg_num)
{
	union float64_components result;
	result.i[0] = current->thread.fp[reg_num * 2];
	result.i[1] = current->thread.fp[reg_num * 2 + 1];
	return 	result.f64;
}

inline void write_fpr64(float64 val, int reg_num)
{
	union float64_components result;
	result.f64 = val;
	current->thread.fp[reg_num * 2] = result.i[0];
	current->thread.fp[reg_num * 2 + 1] = result.i[1];
}
#endif /* __BIG_ENDIAN */

inline float32 read_fpr32l(int reg_num)
{
	return current->thread.fp[reg_num * 2];
}

inline float32 read_fpr32h(int reg_num)
{
	return current->thread.fp[reg_num * 2 + 1];
}

inline void write_fpr32l(float32 val, int reg_num)
{
	current->thread.fp[reg_num * 2] = (unsigned long)val;
}

inline void write_fpr32h(float32 val, int reg_num)
{
	current->thread.fp[reg_num * 2 + 1] = (unsigned long)val;
}
#endif /* CONFIG_CPU_HAS_FPU */
