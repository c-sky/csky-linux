/*
 * linux/arch/csky/math-emu/fp610_inst_op.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014  zhangwm <wenmeng_zhang@c-sky.com>
 */

#include "fp_op.h"
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/thread_info.h>
#include <asm/fpu.h>
#include <asm/siginfo.h>

/*
 * 10 - 14 bits in SOP.
 */
#define SOP_FCMPZ 	0x1
#define SOP_FCMPHSD 	0x2
#define SOP_FCMPLTD 	0x3
#define SOP_FCMPNED 	0x4
#define SOP_FCMPUOD 	0x5
#define SOP_FCMPHSS 	0x6
#define SOP_FCMPLTS 	0x7
#define SOP_FCMPNES 	0x8
#define SOP_FCMPUOS 	0x9
#define SOP_FSTOD 	0xa
#define SOP_FDTOS 	0xb
#define SOP_FSITOD 	0xc
#define SOP_FSITOS 	0xd
#define SOP_FUITOD 	0xe
#define SOP_FUITOS 	0xf
#define SOP_FABSD 	0x10
#define SOP_FABSS 	0x11
#define SOP_FNEGD 	0x12
#define SOP_FNEGS 	0x13
#define SOP_FSQRTD 	0x14
#define SOP_FSQRTS 	0x15
#define SOP_FRECIPD 	0x16
#define SOP_FRECIPS 	0x17
#define SOP_FABSM 	0x18
#define SOP_FNEGM 	0x19
#define SOP_FMOVD 	0x1a
#define SOP_FMOVS 	0x1b

/*
 * 15 - 19 bits in SOP.
 */
#define SOP_FDTOSI 	0x1
#define SOP_FSTOSI 	0x2
#define SOP_FDTOUI 	0x3
#define SOP_FSTOUI 	0x4
#define SOP_FADDD 	0x6
#define SOP_FADDS 	0x7
#define SOP_FSUBD 	0x8
#define SOP_FSUBS 	0x9
#define SOP_FMACD 	0xa
#define SOP_FMACS 	0xb
#define SOP_FMSCD 	0xc
#define SOP_FMSCS 	0xd
#define SOP_FMULD 	0xe
#define SOP_FMULS 	0xf
#define SOP_FDIVD 	0x10
#define SOP_FDIVS 	0x11
#define SOP_FNMACD 	0x12
#define SOP_FNMACS 	0x13
#define SOP_FNMSCD 	0x14
#define SOP_FNMSCS 	0x15
#define SOP_FNMULD 	0x16
#define SOP_FNMULS 	0x17
#define SOP_FADDM 	0x18
#define SOP_FSUBM 	0x19
#define SOP_FMULM 	0x1a
#define SOP_FMACM 	0x1b
#define SOP_FMSCM 	0x1c
#define SOP_FNMACM 	0x1d
#define SOP_FNMSCM	0x1e
#define SOP_FNMULM 	0x1f

#define MASK_19_15_BIT	(0x1f << 15)
#define MASK_4_0_BIT	(0x1f)
#define MASK_14_10_BIT	(0x1f << 10)
#define MASK_9_5_BIT	(0x1f << 5)

/*
 * fpsr.c = (vrx >= 0) ? 1 : 0
 */
static void
fp610_fcmpzhsd(int vrx, struct inst_data *inst_data)
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
fp610_fcmpzhss(int vrx, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_single_constant(0);

	result = float32_le(op_val2, op_val1);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx < 0) ? 1 : 0
 */
static void
fp610_fcmpzltd(int vrx, struct inst_data *inst_data)
{
	float64 op_val1, op_val2;
	flag result;

	op_val1 = get_float64(vrx);
	op_val2 = get_double_constant(0);

	result = float64_lt(op_val1, op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx < 0) ? 1 : 0
 */
static void
fp610_fcmpzlts(int vrx, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_single_constant(0);

	result = float32_lt(op_val1, op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * fpsr.c = (vrx != 0) ? 1 : 0
 */
static void
fp610_fcmpzned(int vrx, struct inst_data *inst_data)
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
fp610_fcmpznes(int vrx, struct inst_data *inst_data)
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
fp610_fcmpzuod(int vrx, struct inst_data *inst_data)
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
fp610_fcmpzuos(int vrx, struct inst_data *inst_data)
{
	float32 op_val1;
	flag result;

	op_val1 = get_float32(vrx);

	result = float32_is_nan(op_val1);

	set_fsr_c(result, inst_data->regs);
}

static void
fp610_fcmpz(int src0, int src1, struct inst_data *inst_data)
{
	switch(src1) {
	case 0x0:
		fp610_fcmpzhsd(src0, inst_data);
		break;
	case 0x4:
		fp610_fcmpzltd(src0, inst_data);
		break;
	case 0x8:
		fp610_fcmpzned(src0, inst_data);
		break;
	case 0xc:
		fp610_fcmpzuod(src0, inst_data);
		break;
	case 0x10:
		fp610_fcmpzhss(src0, inst_data);
		break;
	case 0x14:
		fp610_fcmpzlts(src0, inst_data);
		break;
	case 0x18:
		fp610_fcmpznes(src0, inst_data);
		break;
	case 0x1c:
		fp610_fcmpzuos(src0, inst_data);
		break;
	default:
		break;
	}
}

/*
 * fpsr.c = (vrx >= vry) ? 1 : 0
 */
static void
fp610_fcmphsd(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmphss(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmpltd(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmplts(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmpned(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmpnes(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmpuod(int vrx, int vry, struct inst_data *inst_data)
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
fp610_fcmpuos(int vrx, int vry, struct inst_data *inst_data)
{
	float32 op_val1, op_val2;
	flag result;

	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_is_nan(op_val1) || float32_is_nan(op_val2);

	set_fsr_c(result, inst_data->regs);
}

/*
 * vrz = (double)vrx
 */
static void
fp610_fstod(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	op_val1 = get_float32(vrx);

	result = float32_to_float64(op_val1);

	set_float64(result, vrz);
}

/*
 * vrz = (float)vrx
 */
static void
fp610_fdtos(int vrx, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	float32 result;
	op_val1 = get_float64(vrx);

	result = float64_to_float32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (double)vrx
 */
static void
fp610_fsitod(int vrx, int vrz, struct inst_data *inst_data)
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
fp610_fsitos(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	/* get vrx[31:0] */
	op_val1 = get_float64(vrx);

	result = int32_to_float32((int)op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (double)vrx
 */
static void
fp610_fuitod(int vrx, int vrz, struct inst_data *inst_data)
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
fp610_fuitos(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	float64 result;
	/* get vrx[31:0] */
	op_val1 = get_float64(vrx);

	result = uint32_to_float32((unsigned int)op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = |vrx|
 */
static void
fp610_fabsd(int vrx, int vrz, struct inst_data *inst_data)
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
fp610_fabsm(int vrx, int vrz, struct inst_data *inst_data)
{
	union float64_components u;
	u.f64 = get_float64(vrx);
	u.i[0] &= 0x7fffffff;
	u.i[1] &= 0x7fffffff;
	set_float64(u.f64, vrz);
}

static void
fp610_fabss(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 val = get_float32(vrx);
	val &= 0x7fffffff;
	set_float32(val, vrz);
}

/*
 * vrz = -vrx
 */
static void
fp610_fnegd(int vrx, int vrz, struct inst_data *inst_data)
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
fp610_fnegm(int vrx, int vrz, struct inst_data *inst_data)
{
	union float64_components u;
	u.f64 = get_float64(vrx);
	u.i[0] ^= 0x80000000;
	u.i[1] ^= 0x80000000;
	set_float64(u.f64, vrz);
}

static void
fp610_fnegs(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 val = get_float32(vrx);
	val ^= 0x80000000;
	set_float32(val, vrz);
}

/*
 * vrz = vrx ^ 1/2
 */
static void
fp610_fsqrtd(int vrx, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, result;
	op_val1 = get_float64(vrx);

	result = float64_sqrt(op_val1, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp610_fsqrts(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, result;
	op_val1 = get_float32(vrx);

	result = float32_sqrt(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = 1 / vrx
 */
static void
fp610_frecipd(int vrx, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_double_constant(1);
	op_val2 = get_float64(vrx);

	result = float64_div(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp610_frecips(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_single_constant(1);
	op_val2 = get_float32(vrx);

	result = float32_div(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx
 */
static void
fp610_fmovd(int vrx, int vrz, struct inst_data *inst_data)
{
	float64 result;
	result = get_float64(vrx);

	set_float64(result, vrz);
}

static void
fp610_fmovs(int vrx, int vrz, struct inst_data *inst_data)
{
	float32 result;
	result = get_float32(vrx);

	set_float32(result, vrz);
}

/*
 * vrz = (int)vrx
 */
static void
fp610_fdtosi(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	int result;
	op_val1 = get_float64(vrx);

	switch(vry) {
	case 0x0:
		inst_data->roundData->mode = float_round_nearest_even;
		break;
	case 0x8:
		inst_data->roundData->mode = float_round_to_zero;
		break;
	case 0x10:
		inst_data->roundData->mode = float_round_up;
		break;
	case 0x18:
		inst_data->roundData->mode = float_round_down;
		break;
	default:
		break;
	}

	result = float64_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp610_fstosi(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	switch(vry) {
	case 0x0:
		inst_data->roundData->mode = float_round_nearest_even;
		break;
	case 0x8:
		inst_data->roundData->mode = float_round_to_zero;
		break;
	case 0x10:
		inst_data->roundData->mode = float_round_up;
		break;
	case 0x18:
		inst_data->roundData->mode = float_round_down;
		break;
	default:
		break;
	}

	result = float32_to_int32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = (int)vrx
 */
static void
fp610_fdtoui(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1;
	unsigned int result;
	op_val1 = get_float64(vrx);

	switch(vry) {
	case 0x0:
		inst_data->roundData->mode = float_round_nearest_even;
		break;
	case 0x8:
		inst_data->roundData->mode = float_round_to_zero;
		break;
	case 0x10:
		inst_data->roundData->mode = float_round_up;
		break;
	case 0x18:
		inst_data->roundData->mode = float_round_down;
		break;
	default:
		break;
	}

	result = float64_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

static void
fp610_fstoui(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1;
	int result;
	op_val1 = get_float32(vrx);

	switch(vry) {
	case 0x0:
		inst_data->roundData->mode = float_round_nearest_even;
		break;
	case 0x8:
		inst_data->roundData->mode = float_round_to_zero;
		break;
	case 0x10:
		inst_data->roundData->mode = float_round_up;
		break;
	case 0x18:
		inst_data->roundData->mode = float_round_down;
		break;
	default:
		break;
	}

	result = float32_to_uint32(op_val1, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx + vry
 */
static void
fp610_faddd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_add(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp610_faddm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_add(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_add(op_val1.i[1], op_val2.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp610_fadds(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_add(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx - vry
 */
static void
fp610_fsubd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_sub(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp610_fsubm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_sub(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_sub(op_val1.i[1], op_val2.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp610_fsubs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_sub(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz += vrx * vry
 */
static void
fp610_fmacd(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fmacm(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fmacs(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
 * vrz = vrx * vry - vrz
 */
static void
fp610_fmscd(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fmscm(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fmscs(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
 * vrz = vrx * vry
 */
static void
fp610_fmuld(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_mul(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp610_fmulm(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	union float64_components op_val1, op_val2, result;
	op_val1.f64 = get_float64(vrx);
	op_val2.f64 = get_float64(vry);

	result.i[0] = float32_mul(op_val1.i[0], op_val2.i[0], inst_data->roundData);
	result.i[1] = float32_mul(op_val1.i[1], op_val2.i[1], inst_data->roundData);

	set_float64(result.f64, vrz);
}

static void
fp610_fmuls(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz = vrx / vry
 */
static void
fp610_fdivd(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float64 op_val1, op_val2, result;
	op_val1 = get_float64(vrx);
	op_val2 = get_float64(vry);

	result = float64_div(op_val1, op_val2, inst_data->roundData);

	set_float64(result, vrz);
}

static void
fp610_fdivs(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_div(op_val1, op_val2, inst_data->roundData);

	set_float32(result, vrz);
}

/*
 * vrz -= vrx * vry
 */
static void
fp610_fnmacd(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmacm(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmacs(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
 * vrz = -vrz - vrx * vry
 */
static void
fp610_fnmscd(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmscm(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmscs(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmuld(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmulm(int vrx, int vry, int vrz, struct inst_data *inst_data)
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
fp610_fnmuls(int vrx, int vry, int vrz, struct inst_data *inst_data)
{
	float32 op_val1, op_val2, result;
	op_val1 = get_float32(vrx);
	op_val2 = get_float32(vry);

	result = float32_mul(op_val1, op_val2, inst_data->roundData);
	result ^= 0x80000000;

	set_float32(result, vrz);
}

struct instruction_op_array1{
	void (*const fn)(int vrx, int vrz, struct inst_data *inst_data);
};

struct instruction_op_array1 inst_op1[0x20] = {
	[SOP_FCMPZ] 	= {fp610_fcmpz},
	[SOP_FCMPHSD] 	= {fp610_fcmphsd},
	[SOP_FCMPLTD] 	= {fp610_fcmpltd},
	[SOP_FCMPNED] 	= {fp610_fcmpned},
	[SOP_FCMPUOD] 	= {fp610_fcmpuod},
	[SOP_FCMPHSS] 	= {fp610_fcmphss},
	[SOP_FCMPLTS] 	= {fp610_fcmplts},
	[SOP_FCMPNES] 	= {fp610_fcmpnes},
	[SOP_FCMPUOS] 	= {fp610_fcmpuos},
	[SOP_FSTOD] 	= {fp610_fstod},
	[SOP_FDTOS] 	= {fp610_fdtos},
	[SOP_FSITOD] 	= {fp610_fsitod},
	[SOP_FSITOS] 	= {fp610_fsitos},
	[SOP_FUITOD] 	= {fp610_fuitod},
	[SOP_FUITOS] 	= {fp610_fuitos},
	[SOP_FABSD] 	= {fp610_fabsd},
	[SOP_FABSS] 	= {fp610_fabss},
	[SOP_FNEGD] 	= {fp610_fnegd},
	[SOP_FNEGS] 	= {fp610_fnegs},
	[SOP_FSQRTD] 	= {fp610_fsqrtd},
	[SOP_FSQRTS] 	= {fp610_fsqrts},
	[SOP_FRECIPD] 	= {fp610_frecipd},
	[SOP_FRECIPS] 	= {fp610_frecips},
	[SOP_FABSM] 	= {fp610_fabsm},
	[SOP_FNEGM] 	= {fp610_fnegm},
	[SOP_FMOVD] 	= {fp610_fmovd},
	[SOP_FMOVS] 	= {fp610_fmovs},
};

struct instruction_op_array2{
	void (*const fn)(int vrx, int vry, int vrz, struct inst_data *inst_data);
};

struct instruction_op_array2 inst_op2[0x20] = {
	[SOP_FDTOSI] 	= {fp610_fdtosi},
	[SOP_FSTOSI] 	= {fp610_fstosi},
	[SOP_FDTOUI] 	= {fp610_fdtoui},
	[SOP_FSTOUI] 	= {fp610_fstoui},
	[SOP_FADDD] 	= {fp610_faddd},
	[SOP_FADDS] 	= {fp610_fadds},
	[SOP_FSUBD] 	= {fp610_fsubd},
	[SOP_FSUBS] 	= {fp610_fsubs},
	[SOP_FMACD] 	= {fp610_fmacd},
	[SOP_FMACS] 	= {fp610_fmacs},
	[SOP_FMSCD] 	= {fp610_fmscd},
	[SOP_FMSCS] 	= {fp610_fmscs},
	[SOP_FMULD] 	= {fp610_fmuld},
	[SOP_FMULS] 	= {fp610_fmuls},
	[SOP_FDIVD] 	= {fp610_fdivd},
	[SOP_FDIVS] 	= {fp610_fdivs},
	[SOP_FNMACD] 	= {fp610_fnmacd},
	[SOP_FNMACS] 	= {fp610_fnmacs},
	[SOP_FNMSCD] 	= {fp610_fnmscd},
	[SOP_FNMSCS] 	= {fp610_fnmscs},
	[SOP_FNMULD] 	= {fp610_fnmuld},
	[SOP_FNMULS] 	= {fp610_fnmuls},
	[SOP_FADDM] 	= {fp610_faddm},
	[SOP_FSUBM] 	= {fp610_fsubm},
	[SOP_FMULM] 	= {fp610_fmulm},
	[SOP_FMACM] 	= {fp610_fmacm},
	[SOP_FMSCM] 	= {fp610_fmscm},
	[SOP_FNMACM] 	= {fp610_fnmacm},
	[SOP_FNMSCM] 	= {fp610_fnmscm},
	[SOP_FNMULM] 	= {fp610_fnmulm},
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
	force_sig_info(sig, &info, current);
}

int emulate_610fp_inst(unsigned long inst, struct pt_regs *regs)
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

	/* array1's 19-15 bit is 0, array2's is not 0 */
	if(inst & MASK_19_15_BIT) {
		index = (inst & MASK_19_15_BIT) >> 15;
		vrx = (inst & MASK_4_0_BIT);
		vry = (inst & MASK_14_10_BIT) >> 10;
		vrz = (inst & MASK_9_5_BIT) >> 5;
		if (likely(inst_op2[index].fn != NULL)) {
			inst_op2[index].fn(vrx, vry, vrz, &inst_data);
		} else {
			goto fault;
		}
	} else {
		index = (inst & MASK_14_10_BIT) >> 10;
		vrx = (inst & MASK_4_0_BIT);
		vrz = (inst & MASK_9_5_BIT) >> 5;

		if (likely(inst_op1[index].fn != NULL)) {
			inst_op1[index].fn(vrx, vrz, &inst_data);
		} else {
			goto fault;
		}
	}

	if (round_data.exception != 0) {
		raise_float_exception(round_data.exception);
	}

	return 0;
fault:
	return 1;
}
