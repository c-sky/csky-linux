/*
 * linux/arch/csky/math-emu/fp810_inst_op.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */

#include "fp_op.h"
#include <asm/fpu.h>
#include <asm/siginfo.h>
#include <linux/thread_info.h>
#include <linux/sched.h>
#include <linux/signal.h>

/*
 * 5 - 12 bits in SOP.
 */

#define SOP_FABSD 	0x46
#define SOP_FABSM 	0x86
#define SOP_FABSS 	0x6
#define SOP_FADDD 	0x40
#define SOP_FADDM 	0x80
#define SOP_FADDS 	0x0
#define SOP_FCMPHSD 	0x4c
#define SOP_FCMPHSS 	0xc
#define SOP_FCMPLTD 	0x4d
#define SOP_FCMPLTS 	0xd
#define SOP_FCMPNED 	0x4e
#define SOP_FCMPNES 	0xe
#define SOP_FCMPUOD 	0x4f
#define SOP_FCMPUOS 	0xf
#define SOP_FCMPZHSD 	0x48
#define SOP_FCMPZHSS 	0x8
#define SOP_FCMPZLSD 	0x49
#define SOP_FCMPZLSS 	0x9
#define SOP_FCMPZNED 	0x4a
#define SOP_FCMPZNES 	0xa
#define SOP_FCMPZUOD 	0x4b
#define SOP_FCMPZUOS 	0xb
#define SOP_FDIVD 	0x58
#define SOP_FDIVS 	0x18
#define SOP_FDTOS 	0xd6
#define SOP_FDTOSI_RN 	0xc8
#define SOP_FDTOSI_RZ 	0xc9
#define SOP_FDTOSI_RPI 	0xca
#define SOP_FDTOSI_RNI 	0xcb
#define SOP_FDTOUI_RN 	0xcc
#define SOP_FDTOUI_RZ 	0xcd
#define SOP_FDTOUI_RPI 	0xce
#define SOP_FDTOUI_RNI 	0xcf
#define SOP_FMACD 	0x54
#define SOP_FMACM 	0x94
#define SOP_FMACS 	0x14
#define SOP_FMFVRH 	0xd8
#define SOP_FMFVRL 	0xd9
#define SOP_FMOVD 	0x44
#define SOP_FMOVM 	0x84
#define SOP_FMOVS 	0x4
#define SOP_FMSCD 	0x55
#define SOP_FMSCM 	0x95
#define SOP_FMSCS 	0x15
#define SOP_FMTVRH 	0xda
#define SOP_FMTVRL 	0xdb
#define SOP_FMULD 	0x50
#define SOP_FMULM 	0x90
#define SOP_FMULS 	0x10
#define SOP_FNEGD 	0x47
#define SOP_FNEGM 	0x87
#define SOP_FNEGS 	0x7
#define SOP_FNMACD 	0x56
#define SOP_FNMACM 	0x96
#define SOP_FNMACS 	0x16
#define SOP_FNMSCD 	0x57
#define SOP_FNMSCM 	0x97
#define SOP_FNMSCS 	0x17
#define SOP_FNMULD 	0x51
#define SOP_FNMULM 	0x91
#define SOP_FNMULS 	0x11
#define SOP_FRECIPD 	0x59
#define SOP_FRECIPS 	0x19
#define SOP_FSITOD 	0xd4
#define SOP_FSITOS 	0xd0
#define SOP_FSQRTD 	0x5a
#define SOP_FSQRTS 	0x1a
#define SOP_FSTOD 	0xd7
#define SOP_FSTOSI_RN 	0xc0
#define SOP_FSTOSI_RZ 	0xc1
#define SOP_FSTOSI_RPI 	0xc2
#define SOP_FSTOSI_RNI 	0xc3
#define SOP_FSTOUI_RN 	0xc4
#define SOP_FSTOUI_RZ 	0xc5
#define SOP_FSTOUI_RPI 	0xc6
#define SOP_FSTOUI_RNI 	0xc7
#define SOP_FSUBD 	0x41
#define SOP_FSUBM 	0x81
#define SOP_FSUBS 	0x1
#define SOP_FUITOD 	0xd5
#define SOP_FUITOS 	0xd1


/*
 * 8 - 12 bits in SOP.
 */
#define SOP_FLDD 	0x1
#define SOP_FLDM 	0x2
#define SOP_FLDMD 	0x11
#define SOP_FLDMM 	0x12
#define SOP_FLDMS 	0x10
#define SOP_FLDRD 	0x9
#define SOP_FLDRM 	0xa
#define SOP_FLDRS 	0x8
#define SOP_FLDS	0x0
#define SOP_FSTD      	0x5
#define SOP_FSTM      	0x6
#define SOP_FSTMD	0x15
#define SOP_FSTMM	0x16
#define SOP_FSTMS	0x14
#define SOP_FSTRD 	0xd
#define SOP_FSTRM 	0xe
#define SOP_FSTRS 	0xc
#define SOP_FSTS 	0x4

#define MASK_13_BIT	(1 << 13)
#define MASK_12_5_BIT	(0xff << 5)
#define MASK_19_16_BIT	(0xf << 16)
#define MASK_24_21_BIT	(0xf << 21)
#define MASK_3_0_BIT	(0xf)
#define MASK_12_8_BIT	(0x1f << 8)
#define MASK_20_16_BIT	(0x1f << 16)

/*
 * vrz = |vrx|
 */
static void
fp810_fabsd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components u;
	u.f64 = get_float64(vrx);
#ifdef __BIG_ENDIAN
	u.i[0] &= 0x7fffffff;
#else
	u.i[1] &= 0x7fffffff;
#endif
	set_float64(u.f64, vrz);
}

static void
fp810_fabsm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components u;
	u.f64 = get_float64(vrx);
	u.i[0] &= 0x7fffffff;
	u.i[1] &= 0x7fffffff;
	set_float64(u.f64, vrz);
}

static void
fp810_fabss(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 val = get_float32(vrx);
	val &= 0x7fffffff;
	set_float32(val, vrz);
}

/*
 * vrz = vrx + vry
 */
static void
fp810_faddd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_add(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_faddm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_add(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_add(op_val1.i[1], op_val2.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fadds(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_add(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * fpsr.c = (vrx >= vry) ? 1 : 0
 */
static void
fp810_fcmphsd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_le(op_val2, op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx >= vry) ? 1 : 0
 */
static void
fp810_fcmphss(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_le(op_val2, op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx < vry) ? 1 : 0
 */
static void
fp810_fcmpltd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_lt(op_val1, op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx < vry) ? 1 : 0
 */
static void
fp810_fcmplts(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_lt(op_val1, op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx == vry) ? 0 : 1
 */
static void
fp810_fcmpned(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_eq(op_val1, op_val2);

	set_fsr_c(!result, inst_data->regs);
}

/*
 * fpsr.c = (vrx == vry) ? 0 : 1
 */
static void
fp810_fcmpnes(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_eq(op_val1, op_val2);

	set_fsr_c(!result, inst_data->regs);
}

/*
 * fpsr.c = (vrx == NaN || vry == NaN) ? 1 : 0
 */
static void
fp810_fcmpuod(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_is_nan(op_val1) || float64_is_nan(op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx == NaN || vry == NaN) ? 1 : 0
 */
static void
fp810_fcmpuos(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_is_nan(op_val1) || float32_is_nan(op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx >= 0) ? 1 : 0
 */
static void
fp810_fcmpzhsd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_double_constant(0);

	result = float64_le(op_val2, op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx >= 0) ? 1 : 0
 */
static void
fp810_fcmpzhss(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_single_constant(0);

	result = float32_le(op_val2, op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx <= 0) ? 1 : 0
 */
static void
fp810_fcmpzlsd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_double_constant(0);

	result = float64_le(op_val1, op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx <= 0) ? 1 : 0
 */
static void
fp810_fcmpzlss(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_single_constant(0);

	result = float32_le(op_val1, op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx != 0) ? 1 : 0
 */
static void
fp810_fcmpzned(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_double_constant(0);

	result = float64_eq(op_val1, op_val2);

	set_fsr_c(!result, inst_data->regs);
}

/*
 * fpsr.c = (vrx != 0) ? 1 : 0
 */
static void
fp810_fcmpznes(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_single_constant(0);

	result = float32_eq(op_val1, op_val2);

	set_fsr_c(!result, inst_data->regs);
}

/*
 * fpsr.c = (vrx == NaN) ? 1 : 0
 */
static void
fp810_fcmpzuod(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	flag result;

	op_val1 = get_float64(vrx);

	result = float64_is_nan(op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx == NaN) ? 1 : 0
 */
static void
fp810_fcmpzuos(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	flag result;

	op_val1 = get_float32(vrx);

	result = float32_is_nan(op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * vrz = vrx / vry
 */
static void
fp810_fdivd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_div(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fdivs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_div(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (float)vrx
 */
static void
fp810_fdtos(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	float32 result;
	op_val1 = get_float64(vrx);

	result = float64_to_float32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (int)vrx
 */
static void
fp810_fdtosi_rn(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_nearest_even;
	result = float64_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fdtosi_rz(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_to_zero;
	result = float64_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fdtosi_rpi(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_up;
	result = float64_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fdtosi_rni(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_down;
	result = float64_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (unsigned int)vrx
 */
static void
fp810_fdtoui_rn(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_nearest_even;
	result = float64_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fdtoui_rz(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_to_zero;
	result = float64_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fdtoui_rpi(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_up;
	result = float64_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fdtoui_rni(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	inst_data->roundData->mode = float_round_down;
	result = float64_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz += vrx * vry
 */
static void
fp810_fmacd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, op_val3, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);
	op_val3 = get_float64(vrz);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);
	result = float64_add(result, op_val3, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fmacm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, op_val3, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);
	op_val3.f64 = get_float64(vrz);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[0] = float32_add(result.i[0], op_val3.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);
	result.i[1] = float32_add(result.i[1], op_val3.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fmacs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, op_val3, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);
	op_val3 = get_float32(vrz);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);
	result = float32_add(result, op_val3, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx[63:32]
 */
static void
fp810_fmfvrh(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1;
	float32 result;
	vrx = (inst_data->inst & MASK_20_16_BIT) >> 16;
	op_val1.f64 = get_float64(vrx);

#ifdef __BIG_ENDIAN
	result = (float32)op_val1.i[0];
#else
	result = (float32)op_val1.i[1];
#endif

	set_uint32(result, vrz, inst_data);
}

/*
 * vrz = vrx[31:0]
 */
static void
fp810_fmfvrl(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1;
	float32 result;
	vrx = (inst_data->inst & MASK_20_16_BIT) >> 16;
	op_val1.f64 = get_float64(vrx);

#ifdef __BIG_ENDIAN
	result = (float32)op_val1.i[1];
#else
	result = (float32)op_val1.i[0];
#endif

	set_uint32(result, vrz, inst_data);
}

/*
 * vrz = vrx
 */
static void
fp810_fmovd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	result = get_float64(vrx);

	set_float64(result, vrz);
}

static void
fp810_fmovm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	result = get_float64(vrx);

	set_float64(result, vrz);
}

static void
fp810_fmovs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	result = get_float32(vrx);

	set_float32(result, vrz);
}

/*
 * vrz = vrx * vry - vrz
 */
static void
fp810_fmscd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, op_val3, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);
	op_val3 = get_float64(vrz);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);
	result = float64_sub(result, op_val3, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fmscm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, op_val3, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);
	op_val3.f64 = get_float64(vrz);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[0] = float32_sub(result.i[0], op_val3.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);
	result.i[1] = float32_sub(result.i[1], op_val3.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fmscs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, op_val3, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);
	op_val3 = get_float32(vrz);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);
	result = float32_sub(result, op_val3, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz[63:32] = vrx
 */
static void
fp810_fmtvrh(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components result;
	vrx = (inst_data->inst & MASK_20_16_BIT) >> 16;
#ifdef __BIG_ENDIAN
	result.i[0] = (float32)get_uint32(vrx, inst_data);
	set_float32h(result.i[0], vrz);
#else
	result.i[1] = (float32)get_uint32(vrx, inst_data);
	set_float32h(result.i[1], vrz);
#endif
}

/*
 * vrz[31:0] = vrx
 */
static void
fp810_fmtvrl(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components result;
	vrx = (inst_data->inst & MASK_20_16_BIT) >> 16;
#ifdef __BIG_ENDIAN
	result.i[1] = (float32)get_uint32(vrx, inst_data);
	set_float32(result.i[1], vrz);
#else
	result.i[0] = (float32)get_uint32(vrx, inst_data);
	set_float32(result.i[0], vrz);
#endif
}

/*
 * vrz = vrx * vry
 */
static void
fp810_fmuld(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fmulm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fmuls(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = -vrx
 */
static void
fp810_fnegd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components u;
	u.f64 = get_float64(vrx);
#ifdef __BIG_ENDIAN
	u.i[0] ^= 0x80000000;
#else
	u.i[1] ^= 0x80000000;
#endif
	set_float64(u.f64, vrz);
}

static void
fp810_fnegm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components u;
	u.f64 = get_float64(vrx);
	u.i[0] ^= 0x80000000;
	u.i[1] ^= 0x80000000;
	set_float64(u.f64, vrz);
}

static void
fp810_fnegs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 val = get_float32(vrx);
	val ^= 0x80000000;
	set_float32(val, vrz);
}

/*
 * vrz -= vrx * vry
 */
static void
fp810_fnmacd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, op_val3, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);
	op_val3 = get_float64(vrz);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);
	result = float64_sub(op_val3, result, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fnmacm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, op_val3, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);
	op_val3.f64 = get_float64(vrz);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[0] = float32_sub(op_val3.i[0], result.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);
	result.i[1] = float32_sub(op_val3.i[1], result.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fnmacs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, op_val3, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);
	op_val3 = get_float32(vrz);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);
	result = float32_sub(op_val3, result, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = -vrz -vrx * vry
 */
static void
fp810_fnmscd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, op_val3, result;
	union float64_components tmp;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);
	op_val3 = get_float64(vrz);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);
	tmp.f64 = op_val3;
#ifdef __BIG_ENDIAN
	tmp.i[0] ^= 0x80000000;
#else
	tmp.i[1] ^= 0x80000000;
#endif
	result = float64_sub(tmp.f64, result, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fnmscm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, op_val3, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);
	op_val3.f64 = get_float64(vrz);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);
	op_val3.i[0] ^= 0x80000000;
	op_val3.i[1] ^= 0x80000000;
	result.i[0] = float32_sub(op_val3.i[0], result.i[0], inst_data->roundData);
	result.i[1] = float32_sub(op_val3.i[1], result.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fnmscs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, op_val3, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);
	op_val3 = get_float32(vrz);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);
	op_val3 ^= 0x80000000;
	result = float32_sub(op_val3, result, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = -vrx * vry
 */
static void
fp810_fnmuld(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	union float64_components tmp;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);
	tmp.f64 = result;
#ifdef __BIG_ENDIAN
	tmp.i[0] ^= 0x80000000;
#else
	tmp.i[1] ^= 0x80000000;
#endif

	set_float64(tmp.f64, vrz);
}

static void
fp810_fnmulm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);
	result.i[0] ^= 0x80000000;
	result.i[1] ^= 0x80000000;

	set_float64(result.f64, vrz);
}

static void
fp810_fnmuls(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);
	result ^= 0x80000000;

	set_float32(result, vrz);
}

/*
 * vrz = 1 / vrx
 */
static void
fp810_frecipd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_double_constant(1);
	op_val2 = get_float64(vrx);

	result = float64_div(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_frecips(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_single_constant(1);
	op_val2 = get_float32(vrx);

	result = float32_div(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (double)vrx
 */
static void
fp810_fsitod(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	/* get vrx[31:0] */
	op_val1 = get_float32(vrx);

	result = int32_to_float64((int)op_val1);

	set_float64(result, vrz);
}

/*
 * vrz = (float)vrx
 */
static void
fp810_fsitos(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	/* get vrx[31:0] */
	op_val1 = get_float64(vrx);

	result = int32_to_float32((int)op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx ^ 1/2
 */
static void
fp810_fsqrtd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, result;
	op_val1 = get_float64(vrx);

	result = float64_sqrt(op_val1, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fsqrts(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, result;
	op_val1 = get_float32(vrx);

	result = float32_sqrt(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (double)vrx
 */
static void
fp810_fstod(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	op_val1 = get_float32(vrx);

	result = float32_to_float64(op_val1);

	set_float64(result, vrz);
}

/*
 * vrz = (int)vrx
 */
static void
fp810_fstosi_rn(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_nearest_even;
	result = float32_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fstosi_rz(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_to_zero;
	result = float32_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fstosi_rpi(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_up;
	result = float32_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fstosi_rni(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_down;
	result = float32_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (unsigned int)vrx
 */
static void
fp810_fstoui_rn(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_nearest_even;
	result = float32_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fstoui_rz(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_to_zero;
	result = float32_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fstoui_rpi(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_up;
	result = float32_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp810_fstoui_rni(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	inst_data->roundData->mode = float_round_down;
	result = float32_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx - vry
 */
static void
fp810_fsubd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_sub(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp810_fsubm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_sub(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_sub(op_val1.i[1], op_val2.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp810_fsubs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_sub(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (double)vrx
 */
static void
fp810_fuitod(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	/* get vrx[31:0] */
	op_val1 = get_float32(vrx);

	result = int32_to_float64((unsigned int)op_val1);

	set_float64(result, vrz);
}

/*
 * vrz = (float)vrx
 */
static void
fp810_fuitos(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	/* get vrx[31:0] */
	op_val1 = get_float64(vrx);

	result = uint32_to_float32((unsigned int)op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = *(vrx + imm * 4)
 */
static void
fp810_fldd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	imm = get_imm8(inst_data);
	result = get_float64_from_memory(op_val1 + imm * 4);

	set_float64(result, vrz);
}

static void
fp810_fldm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	imm = get_imm8(inst_data);
	result = get_float64_from_memory(op_val1 + imm * 8);

	set_float64(result, vrz);
}

static void
fp810_flds(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	unsigned int imm;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	imm = get_imm8(inst_data);
	result = get_float32_from_memory(op_val1 + imm * 4);

	set_float32(result, vrz);
}

/*
 * vrz = *(vrx)  ...
 */
static void
fp810_fldmd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	int i;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	for(i = 0; i < vry; i++) {
		result = get_float64_from_memory(op_val1 + i * 8);
		set_float64(result, vrz + i);
	}
}

static void
fp810_fldmm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	int i;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	for(i = 0; i < vry; i++) {
		result = get_float64_from_memory(op_val1 + i * 8);
		set_float64(result, vrz + i);
	}
}

static void
fp810_fldms(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	int i;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	for(i = 0; i < vry; i++) {
		result = get_float32_from_memory(op_val1 + i * 4);
		set_float32(result, vrz + i);
	}
}

/*
 * vrz = *(vrx + vry * imm)
 */
static void
fp810_fldrd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm, op_val1, op_val2;
	imm = get_imm4l(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	op_val2 = get_uint32(vry, inst_data);
	result = get_float64_from_memory(op_val1 + (op_val2 << imm));

	set_float64(result, vrz);
}

static void
fp810_fldrm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm, op_val1, op_val2;
	imm = get_imm4l(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	op_val2 = get_uint32(vry, inst_data);
	result = get_float64_from_memory(op_val1 + (op_val2 << imm));

	set_float64(result, vrz);
}

static void
fp810_fldrs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	unsigned int imm, op_val1, op_val2;
	imm = get_imm4l(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	op_val2 = get_uint32(vry, inst_data);
	result = get_float32_from_memory(op_val1 + (op_val2 << imm));

	set_float32(result, vrz);
}

/*
 * *(vrx + imm * 4) = vrz
 */
static void
fp810_fstd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm, op_val1;
	imm = get_imm8(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	result = get_float64(vrz);

	set_float64_to_memory(result, op_val1 + imm * 4);
}

static void
fp810_fstm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm, op_val1;
	imm = get_imm8(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	result = get_float64(vrz);

	set_float64_to_memory(result, op_val1 + imm * 8);
}

static void
fp810_fsts(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	unsigned int imm, op_val1;
	imm = get_imm8(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	result = get_float32(vrz);

	set_float32_to_memory(result, op_val1 + imm * 4);
}

/*
 * vrz = *(vrx)  ...
 */
static void
fp810_fstmd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	int i;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	for(i = 0; i < vry; i++) {
		result = get_float64(vrz + i);
		set_float64_to_memory(result, op_val1 + i * 8);
	}
}

static void
fp810_fstmm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	int i;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	for(i = 0; i < vry; i++) {
		result = get_float64(vrz + i);
		set_float64_to_memory(result, op_val1 + i * 8);
	}
}

static void
fp810_fstms(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	int i;
	unsigned int op_val1;

	op_val1 = get_uint32(vrx, inst_data);
	for(i = 0; i < vry; i++) {
		result = get_float32(vrz + i);
		set_float32_to_memory(result, op_val1 + i * 4);
	}
}

/*
 * *(vrx + vry * imm) = vrz
 */
static void
fp810_fstrd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm, op_val1, op_val2;
	imm = get_imm4l(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	op_val2 = get_uint32(vry, inst_data);
	result = get_float64(vrz);

	set_float64_to_memory(result, op_val1 + (op_val2 << imm));
}

static void
fp810_fstrm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 result;
	unsigned int imm, op_val1, op_val2;
	imm = get_imm4l(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	op_val2 = get_uint32(vry, inst_data);
	result = get_float64(vrz);

	set_float64_to_memory(result, op_val1 + (op_val2 << imm));
}

static void
fp810_fstrs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 result;
	unsigned int imm,op_val1, op_val2;
	imm = get_imm4l(inst_data);
	op_val1 = get_uint32(vrx, inst_data);
	op_val2 = get_uint32(vry, inst_data);
	result = get_float32(vrz);

	set_float32_to_memory(result, op_val1 + (op_val2 << imm));
}

struct instruction_op_array{
	void (*const fn)(int vrx, int vry, int vrz, struct inst_data *inst_data);
};

struct instruction_op_array inst_op1[0xff] = {
	[SOP_FABSD] 	= {fp810_fabsd},
	[SOP_FABSM] 	= {fp810_fabsm},
	[SOP_FABSS] 	= {fp810_fabss},
	[SOP_FADDD] 	= {fp810_faddd},
	[SOP_FADDM] 	= {fp810_faddm},
	[SOP_FADDS] 	= {fp810_fadds},
	[SOP_FCMPHSD] 	= {fp810_fcmphsd},
	[SOP_FCMPHSS] 	= {fp810_fcmphss},
	[SOP_FCMPLTD] 	= {fp810_fcmpltd},
	[SOP_FCMPLTS] 	= {fp810_fcmplts},
	[SOP_FCMPNED] 	= {fp810_fcmpned},
	[SOP_FCMPNES] 	= {fp810_fcmpnes},
	[SOP_FCMPUOD] 	= {fp810_fcmpuod},
	[SOP_FCMPUOS] 	= {fp810_fcmpuos},
	[SOP_FCMPZHSD] 	= {fp810_fcmpzhsd},
	[SOP_FCMPZHSS] 	= {fp810_fcmpzhss},
	[SOP_FCMPZLSD] 	= {fp810_fcmpzlsd},
	[SOP_FCMPZLSS] 	= {fp810_fcmpzlss},
	[SOP_FCMPZNED] 	= {fp810_fcmpzned},
	[SOP_FCMPZNES] 	= {fp810_fcmpznes},
	[SOP_FCMPZUOD] 	= {fp810_fcmpzuod},
	[SOP_FCMPZUOS] 	= {fp810_fcmpzuos},
	[SOP_FDIVD] 	= {fp810_fdivd},
	[SOP_FDIVS] 	= {fp810_fdivs},
	[SOP_FDTOS] 	= {fp810_fdtos},
	[SOP_FDTOSI_RN] 	= {fp810_fdtosi_rn},
	[SOP_FDTOSI_RZ] 	= {fp810_fdtosi_rz},
	[SOP_FDTOSI_RPI] 	= {fp810_fdtosi_rpi},
	[SOP_FDTOSI_RNI] 	= {fp810_fdtosi_rni},
	[SOP_FDTOUI_RN] 	= {fp810_fdtoui_rn},
	[SOP_FDTOUI_RZ] 	= {fp810_fdtoui_rz},
	[SOP_FDTOUI_RPI] 	= {fp810_fdtoui_rpi},
	[SOP_FDTOUI_RNI] 	= {fp810_fdtoui_rni},
	[SOP_FMACD] 	= {fp810_fmacd},
	[SOP_FMACM] 	= {fp810_fmacm},
	[SOP_FMACS] 	= {fp810_fmacs},
	[SOP_FMFVRH] 	= {fp810_fmfvrh},
	[SOP_FMFVRL] 	= {fp810_fmfvrl},
	[SOP_FMOVD] 	= {fp810_fmovd},
	[SOP_FMOVM] 	= {fp810_fmovm},
	[SOP_FMOVS] 	= {fp810_fmovs},
	[SOP_FMSCD] 	= {fp810_fmscd},
	[SOP_FMSCM] 	= {fp810_fmscm},
	[SOP_FMSCS] 	= {fp810_fmscs},
	[SOP_FMTVRH] 	= {fp810_fmtvrh},
	[SOP_FMTVRL] 	= {fp810_fmtvrl},
	[SOP_FMULD] 	= {fp810_fmuld},
	[SOP_FMULM] 	= {fp810_fmulm},
	[SOP_FMULS] 	= {fp810_fmuls},
	[SOP_FNEGD] 	= {fp810_fnegd},
	[SOP_FNEGM] 	= {fp810_fnegm},
	[SOP_FNEGS] 	= {fp810_fnegs},
	[SOP_FNMACD] 	= {fp810_fnmacd},
	[SOP_FNMACM] 	= {fp810_fnmacm},
	[SOP_FNMACS] 	= {fp810_fnmacs},
	[SOP_FNMSCD] 	= {fp810_fnmscd},
	[SOP_FNMSCM] 	= {fp810_fnmscm},
	[SOP_FNMSCS] 	= {fp810_fnmscs},
	[SOP_FNMULD] 	= {fp810_fnmuld},
	[SOP_FNMULM] 	= {fp810_fnmulm},
	[SOP_FNMULS] 	= {fp810_fnmuls},
	[SOP_FRECIPD] 	= {fp810_frecipd},
	[SOP_FRECIPS] 	= {fp810_frecips},
	[SOP_FSITOD] 	= {fp810_fsitod},
	[SOP_FSITOS] 	= {fp810_fsitos},
	[SOP_FSQRTD] 	= {fp810_fsqrtd},
	[SOP_FSQRTS] 	= {fp810_fsqrts},
	[SOP_FSTOD] 	= {fp810_fstod},
	[SOP_FSTOSI_RN] 	= {fp810_fstosi_rn},
	[SOP_FSTOSI_RZ] 	= {fp810_fstosi_rz},
	[SOP_FSTOSI_RPI] 	= {fp810_fstosi_rpi},
	[SOP_FSTOSI_RNI] 	= {fp810_fstosi_rni},
	[SOP_FSTOUI_RN] 	= {fp810_fstoui_rn},
	[SOP_FSTOUI_RZ] 	= {fp810_fstoui_rz},
	[SOP_FSTOUI_RPI] 	= {fp810_fstoui_rpi},
	[SOP_FSTOUI_RNI] 	= {fp810_fstoui_rni},
	[SOP_FSUBD] 	= {fp810_fsubd},
	[SOP_FSUBM] 	= {fp810_fsubm},
	[SOP_FSUBS] 	= {fp810_fsubs},
	[SOP_FUITOD] 	= {fp810_fuitod},
	[SOP_FUITOS] 	= {fp810_fuitos},
};

struct instruction_op_array inst_op2[0x1f] = {
	[SOP_FLDD] 	= {fp810_fldd},
	[SOP_FLDM] 	= {fp810_fldm},
	[SOP_FLDMD] 	= {fp810_fldmd},
	[SOP_FLDMM] 	= {fp810_fldmm},
	[SOP_FLDMS] 	= {fp810_fldms},
	[SOP_FLDRD] 	= {fp810_fldrd},
	[SOP_FLDRM] 	= {fp810_fldrm},
	[SOP_FLDRS] 	= {fp810_fldrs},
	[SOP_FLDS] 	= {fp810_flds},
	[SOP_FSTD] 	= {fp810_fstd},
	[SOP_FSTM] 	= {fp810_fstm},
	[SOP_FSTMD] 	= {fp810_fstmd},
	[SOP_FSTMM] 	= {fp810_fstmm},
	[SOP_FSTMS] 	= {fp810_fstms},
	[SOP_FSTRD] 	= {fp810_fstrd},
	[SOP_FSTRM] 	= {fp810_fstrm},
	[SOP_FSTRS] 	= {fp810_fstrs},
	[SOP_FSTS] 	= {fp810_fsts},
};

static unsigned int fpe_exception_pc;

void raise_float_exception(unsigned int exception)
{
	int sig;
	siginfo_t info;
	unsigned int enable_ex = exception & read_fpcr() & FPE_REGULAR_EXCEPTION;

	if (!enable_ex) {
		return;
	}

	if (!(exception & FPE_REGULAR_EXCEPTION)) {
		info.si_code = __SI_FAULT;
		goto send_sigfpe;
	}

	if(enable_ex & FPE_IOC){
		info.si_code = FPE_FLTINV;
	}
	else if(enable_ex & FPE_DZC){
		info.si_code = FPE_FLTDIV;
	}
	else if(enable_ex & FPE_UFC){
		info.si_code = FPE_FLTUND;
	}
	else if(enable_ex & FPE_OFC){
		info.si_code = FPE_FLTOVF;
	}
	else if(enable_ex & FPE_IXC){
		info.si_code = FPE_FLTRES;
	}

send_sigfpe:
	sig = SIGFPE;
	info.si_signo = SIGFPE;
	info.si_errno = 0;
	info.si_addr = (void *)(fpe_exception_pc);
	send_sig_info(sig, &info, current);
}

int emulate_810fp_inst(unsigned long inst, struct pt_regs *regs)
{
	int index, vrx, vry, vrz;
	struct inst_data inst_data;
	struct roundingData round_data;

	fpe_exception_pc = regs->pc;
	round_data.mode = get_round_mode();
	round_data.exception = 0;
	inst_data.inst = inst;
	inst_data.roundData = &round_data;
	inst_data.regs = regs;

	/* array1's 13 bit is 0, array2's is 1 */
	if(inst & MASK_13_BIT) {
		index = (inst & MASK_12_8_BIT) >> 8;
		vrx = (inst & MASK_20_16_BIT) >> 16;
		vry = (inst & MASK_24_21_BIT) >> 21;
		vrz = (inst & MASK_3_0_BIT);

		if(likely(inst_op2[index].fn != NULL)) {
			inst_op2[index].fn(vrx, vry, vrz, &inst_data);
		} else {
			goto fault;
		}
	} else {
		index = (inst & MASK_12_5_BIT) >> 5;
		vrx = (inst & MASK_19_16_BIT) >> 16;
		vry = (inst & MASK_24_21_BIT) >> 21;
		vrz = (inst & MASK_3_0_BIT);

		if(likely(inst_op1[index].fn != NULL)) {
			inst_op1[index].fn(vrx, vry, vrz, &inst_data);
		} else {
			goto fault;
		}
	}

	if(round_data.exception != 0) {
		raise_float_exception(round_data.exception);
	}

	return 0;
fault:
	return 1;
}
