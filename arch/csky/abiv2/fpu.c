// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/ptrace.h>
#include <linux/uaccess.h>

#define CONFIG_FCR 0

inline unsigned int
read_pt_regs(unsigned int rx, struct pt_regs *regs)
{
	unsigned int value;

	if(rx < 14)
		value  = *((int *)regs + rx + 3);
	else if(rx == 14)
		if(user_mode(regs))
			asm volatile("mfcr %0, cr<14, 1>\n":"=r"(value));
		else
			value = sizeof(struct pt_regs) + ((unsigned int)regs);
	else
		value = *((int *)regs + rx + 2);

	return value;
}

inline void
write_pt_regs(unsigned int value, unsigned int rx, struct pt_regs *regs)
{
	if(rx < 14)
		*((int *)regs + rx + 3) = value;
	else if(rx == 14)
		if(user_mode(regs))
			asm volatile("mtcr %0, cr<14, 1>\n"::"r"(value));
		else
			printk("math emulate trying to write sp.\n");
	else
		*((int *)regs + rx + 2) = value;
}

void __init init_fpu(void)
{
	unsigned long fcr;

	fcr = CONFIG_FCR;
	asm volatile("mtcr %0, cr<1, 2>\n"::"r"(fcr));
}

inline unsigned int read_fpcr(void)
{
	unsigned int result = 0;
	asm volatile("mfcr %0, cr<1, 2>\n":"=r"(result));
	return result;
}

inline void write_fpcr(unsigned int val)
{
	unsigned int result = val | CONFIG_FCR;
	asm volatile("mtcr %0, cr<1, 2>\n"::"r"(result));
}

inline unsigned int read_fpesr(void)
{
	unsigned int result = 0;
	asm volatile("mfcr %0, cr<2, 2>\n":"=r"(result));
	return result;
}

inline void write_fpesr(unsigned int val)
{
	unsigned int result = val;
	asm volatile("mtcr %0, cr<2, 2>\n"::"r"(result));
}

/* use as fpc control reg read/write in glibc. */
int fpu_libc_helper(struct pt_regs * regs)
{
	mm_segment_t fs;
	unsigned long instrptr, regx = 0;
	unsigned int fault;

	u16 instr_hi, instr_low;
	unsigned long index_regx = 0, index_fpregx_prev = 0, index_fpregx_next = 0;
	unsigned long tinstr = 0;

	instrptr = instruction_pointer(regs);

	/* CSKYV2's 32 bit instruction may not align 4 words */
	fs = get_fs();
	set_fs(KERNEL_DS);
	fault = __get_user(instr_low, (u16 *)(instrptr & ~1));
	set_fs(fs);
	if (fault) {
		goto bad_or_fault;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	fault = __get_user(instr_hi, (u16 *)((instrptr + 2) & ~1));
	set_fs(fs);
	if (fault) {
		goto bad_or_fault;
	}

	tinstr = instr_hi | ((unsigned long)instr_low << 16);

	index_fpregx_next = ((tinstr >> 21) & 0x1F);

	/* just want to handle instruction which opration cr<1, 2> or cr<2, 2> */
	if(index_fpregx_next != 2){
		goto bad_or_fault;
	}

	/*
	 * define four macro to distinguish the instruction is mfcr or mtcr.
	 */
#define MTCR_MASK 0xFC00FFE0
#define MFCR_MASK 0xFC00FFE0
#define MTCR_DISTI 0xC0006420
#define MFCR_DISTI 0xC0006020

	if ((tinstr & MTCR_MASK) == MTCR_DISTI)
	{
		index_regx = (tinstr >> 16) & 0x1F;
		index_fpregx_prev = tinstr & 0x1F;

		regx = read_pt_regs(index_regx, regs);

		if(index_fpregx_prev == 1) {
			write_fpcr(regx);
		} else if (index_fpregx_prev == 2) {
			write_fpesr(regx);
		} else {
			goto bad_or_fault;
		}

		regs->pc +=4;
		return 1;
	} else if ((tinstr & MFCR_MASK) == MFCR_DISTI) {
		index_regx = tinstr & 0x1F;
		index_fpregx_prev = ((tinstr >> 16) & 0x1F);

		if (index_fpregx_prev == 1) {
			regx = read_fpcr();
		} else if (index_fpregx_prev == 2) {
			regx = read_fpesr();
		} else {
			goto bad_or_fault;
		}

		write_pt_regs(regx, index_regx, regs);

		regs->pc +=4;
		return 1;
	}

bad_or_fault:
	return 0;
}

void fpu_fpe(struct pt_regs * regs)
{
	int sig;
	unsigned int fesr;
	siginfo_t info;
	asm volatile("mfcr %0, cr<2, 2>":"=r"(fesr));

	if(fesr & FPE_ILLE){
		info.si_code = ILL_ILLOPC;
		sig = SIGILL;
	}
	else if(fesr & FPE_IDC){
		info.si_code = ILL_ILLOPN;
		sig = SIGILL;
	}
	else if(fesr & FPE_FEC){
		sig = SIGFPE;
		if(fesr & FPE_IOC){
			info.si_code = FPE_FLTINV;
		}
		else if(fesr & FPE_DZC){
			info.si_code = FPE_FLTDIV;
		}
		else if(fesr & FPE_UFC){
			info.si_code = FPE_FLTUND;
		}
		else if(fesr & FPE_OFC){
			info.si_code = FPE_FLTOVF;
		}
		else if(fesr & FPE_IXC){
			info.si_code = FPE_FLTRES;
		}
		else {
			info.si_code = NSIGFPE;
		}
	}
	else {
		info.si_code = NSIGFPE;
		sig = SIGFPE;
	}
	info.si_signo = SIGFPE;
	info.si_errno = 0;
	info.si_addr = (void *)regs->pc;
	force_sig_info(sig, &info, current);
}

#define FMFVR_FPU_REGS(vrx, vry) \
	"fmfvrl %0, "#vrx" \n" \
	"fmfvrh %1, "#vrx" \n" \
	"fmfvrl %2, "#vry" \n" \
	"fmfvrh %3, "#vry" \n"

#define FMTVR_FPU_REGS(vrx, vry) \
	"fmtvrl "#vrx", %0 \n" \
	"fmtvrh "#vrx", %1 \n" \
	"fmtvrl "#vry", %2 \n" \
	"fmtvrh "#vry", %3 \n"

#define STW_FPU_REGS(a, b, c, d) \
	"stw    %0, (%4, "#a") \n" \
	"stw    %1, (%4, "#b") \n" \
	"stw    %2, (%4, "#c") \n" \
	"stw    %3, (%4, "#d") \n"

#define LDW_FPU_REGS(a, b, c, d) \
	"ldw    %0, (%4, "#a") \n" \
	"ldw    %1, (%4, "#b") \n" \
	"ldw    %2, (%4, "#c") \n" \
	"ldw    %3, (%4, "#d") \n"


void save_to_user_fp(struct user_fp *user_fp)
{
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	unsigned long *fpregs;

	local_save_flags(flg);


	asm volatile(
		"mfcr    %0, cr<1, 2> \n"
		"mfcr    %1, cr<2, 2> \n"
		:"=r"(tmp1),"=r"(tmp2));

	user_fp->fcr = tmp1;
	user_fp->fesr = tmp2;

	fpregs = &user_fp->vr[0];
	asm volatile(
		FMFVR_FPU_REGS(vr0, vr1)
		STW_FPU_REGS(0, 4, 16, 20)
		FMFVR_FPU_REGS(vr2, vr3)
		STW_FPU_REGS(32, 36, 48, 52)
		FMFVR_FPU_REGS(vr4, vr5)
		STW_FPU_REGS(64, 68, 80, 84)
		FMFVR_FPU_REGS(vr6, vr7)
		STW_FPU_REGS(96, 100, 112, 116)
		"addi    %4, 128\n"
		FMFVR_FPU_REGS(vr8, vr9)
		STW_FPU_REGS(0, 4, 16, 20)
		FMFVR_FPU_REGS(vr10, vr11)
		STW_FPU_REGS(32, 36, 48, 52)
		FMFVR_FPU_REGS(vr12, vr13)
		STW_FPU_REGS(64, 68, 80, 84)
		FMFVR_FPU_REGS(vr14, vr15)
		STW_FPU_REGS(96, 100, 112, 116)
		:"=a"(tmp1),"=a"(tmp2),"=a"(tmp3),
		"=a"(tmp4),"+a"(fpregs));

	local_irq_restore(flg);
}

void restore_from_user_fp(struct user_fp *user_fp)
{
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	unsigned long *fpregs;

	local_irq_save(flg);

	tmp1 = user_fp->fcr;
	tmp2 = user_fp->fesr;

	asm volatile(
		"mtcr   %0, cr<1, 2>\n"
		"mtcr   %1, cr<2, 2>\n"
		::"r"(tmp1), "r"(tmp2));

	fpregs = &user_fp->vr[0];
	asm volatile(
		LDW_FPU_REGS(0, 4, 16, 20)
		FMTVR_FPU_REGS(vr0, vr1)
		LDW_FPU_REGS(32, 36, 48, 52)
		FMTVR_FPU_REGS(vr2, vr3)
		LDW_FPU_REGS(64, 68, 80, 84)
		FMTVR_FPU_REGS(vr4, vr5)
		LDW_FPU_REGS(96, 100, 112, 116)
		FMTVR_FPU_REGS(vr6, vr7)
		"addi   %4, 128\n"
		LDW_FPU_REGS(0, 4, 16, 20)
		FMTVR_FPU_REGS(vr8, vr9)
		LDW_FPU_REGS(32, 36, 48, 52)
		FMTVR_FPU_REGS(vr10, vr11)
		LDW_FPU_REGS(64, 68, 80, 84)
		FMTVR_FPU_REGS(vr12, vr13)
		LDW_FPU_REGS(96, 100, 112, 116)
		FMTVR_FPU_REGS(vr14, vr15)
		:"=a"(tmp1),"=a"(tmp2),"=a"(tmp3),
		"=a"(tmp4),"+a"(fpregs));

	local_irq_restore(flg);
}


