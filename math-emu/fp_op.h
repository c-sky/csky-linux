/*
 * linux/arch/csky/math-emu/fp_op.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */


#ifndef __CSKY_FP_OP_H__
#define __CSKY_FP_OP_H__

#include "softfloat.h"

struct inst_data {
	struct roundingData *roundData;
	unsigned int inst;
	struct pt_regs* regs;
};

union float64_components {
	float64 f64;
	unsigned int i[2];
};

void raise_float_exception(unsigned int exception);
inline unsigned int get_fp810_instruction(struct pt_regs *regs);
inline unsigned int get_imm8(struct inst_data *inst_data);
inline unsigned int get_imm4l(struct inst_data *inst_data);
inline flag get_fsr_c(struct pt_regs* regs);
inline void set_fsr_c(unsigned int val, struct pt_regs* regs);
inline float64 read_fpr64(int reg_num);
inline float32 read_fpr32l(int reg_num);
inline float32 read_fpr32h(int reg_num);
inline void write_fpr64(float64 val, int reg_num);
inline void write_fpr32l(float32 val, int reg_num);
inline void write_fpr32h(float32 val, int reg_num);
inline unsigned int read_gr(int reg_num, struct pt_regs *regs);
inline void write_gr(unsigned int val, int reg_num, struct pt_regs *regs);
inline float32 read_fpr(int reg_num);
inline void write_fpr(float32 val, int reg_num);
inline float32 read_fpsr(void);
inline void write_fpsr(float32 val);
inline float32 get_fpvalue32(unsigned int addr);
inline void set_fpvalue32(unsigned int val, unsigned int addr);
inline float64 get_fpvalue64(unsigned int addr);
inline void set_fpvalue64(float64 val, unsigned int addr);
inline unsigned int read_fpcr(void);
inline void write_fpcr(unsigned int val);
inline unsigned int read_fpesr(void);
inline void write_fpesr(unsigned int val);
inline float64 get_double_constant(const unsigned int index);
inline float32 get_single_constant(const unsigned int index);
inline unsigned int get_round_mode(void);
inline void set_round_mode(unsigned int val);
inline void clear_fesr(unsigned int fesr);
inline float64 get_float64(int reg_num);
inline float32 get_float32(int reg_num);
inline void set_float64(float64 val, int reg_num);
inline void set_float32(float32 val, int reg_num);
inline void set_float32h(float32 val, int reg_num);
inline unsigned int get_uint32(int reg_num, struct inst_data *inst_data);
inline void set_uint32(unsigned int val, int reg_num, struct inst_data *inst_data);
inline float64 get_float64_from_memory(unsigned long addr);
inline void set_float64_to_memory(float64 val, unsigned long addr);
inline float32 get_float32_from_memory(unsigned long addr);
inline void set_float32_to_memory(float32 val, unsigned long addr);
#endif
