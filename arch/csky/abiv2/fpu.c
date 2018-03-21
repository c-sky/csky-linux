// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/ptrace.h>
#include <linux/uaccess.h>

#if 0 /* FIXME: to support fpu exceptions */
#define CONFIG_FCR (IDE_STAT | IXE_STAT | UFE_STAT |\
		    OFE_STAT | DZE_STAT | IOE_STAT)
#else
#define CONFIG_FCR 0
#endif

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

typedef struct fpregset {
	int f_fcr;
	int f_fsr;		/* Nothing in CPU_CSKYV2 */
	int f_fesr;
	int f_feinst1;		/* Nothing in CPU_CSKYV2 */
	int f_feinst2;		/* Nothing in CPU_CSKYV2 */
	int f_fpregs[32];
} fpregset_t;

int save_fpu_state(struct sigcontext *sc, struct pt_regs *regs)
{
	int err = 0;
	fpregset_t fpregs;
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	int * fpgr;

	local_irq_save(flg);
	fpgr = &(fpregs.f_fpregs[0]);
	asm volatile(
		"mfcr    %0, cr<1, 2>\n"
		"mfcr    %1, cr<2, 2>\n"
		:"=r"(fpregs.f_fcr),"=r"(fpregs.f_fesr));

	asm volatile(
		FMFVR_FPU_REGS(vr0, vr1)
		STW_FPU_REGS(0, 4, 8, 12)
		FMFVR_FPU_REGS(vr2, vr3)
		STW_FPU_REGS(16, 20, 24, 28)
		FMFVR_FPU_REGS(vr4, vr5)
		STW_FPU_REGS(32, 36, 40, 44)
		FMFVR_FPU_REGS(vr6, vr7)
		STW_FPU_REGS(48, 52, 56, 60)
		"addi    %4, 32\n"
		"addi    %4, 32\n"
		FMFVR_FPU_REGS(vr8, vr9)
		STW_FPU_REGS(0, 4, 8, 12)
		FMFVR_FPU_REGS(vr10, vr11)
		STW_FPU_REGS(16, 20, 24, 28)
		FMFVR_FPU_REGS(vr12, vr13)
		STW_FPU_REGS(32, 36, 40, 44)
		FMFVR_FPU_REGS(vr14, vr15)
		STW_FPU_REGS(48, 52, 56, 60)
		:"=a"(tmp1),"=a"(tmp2),"=a"(tmp3),
		"=a"(tmp4),"+a"(fpgr));
	local_irq_restore(flg);

	err |= copy_to_user(&sc->sc_fcr, &fpregs, sizeof(fpregs));
	return err;
}

int restore_fpu_state(struct sigcontext *sc)
{
	int err = 0;
	fpregset_t fpregs;
	unsigned long flg;
	unsigned long tmp1, tmp2, tmp3, tmp4;
	unsigned long fctl0, fctl1, fctl2;
	int * fpgr;

	if (__copy_from_user(&fpregs, &sc->sc_fcr, sizeof(fpregs)))
	{
		err = 1;
		goto out;
	}

	local_irq_save(flg);
	fctl0 = fpregs.f_fcr;
	fctl1 = fpregs.f_fsr;
	fctl2 = fpregs.f_fesr;
	fpgr = &(fpregs.f_fpregs[0]);
	asm volatile(
		"mtcr   %0, cr<1, 2>\n"
		"mtcr   %1, cr<2, 2>\n"
		::"r"(fctl0), "r"(fctl2));

	asm volatile(
		LDW_FPU_REGS(0, 4, 8, 12)
		FMTVR_FPU_REGS(vr0, vr1)
		LDW_FPU_REGS(16, 20, 24, 28)
		FMTVR_FPU_REGS(vr2, vr3)
		LDW_FPU_REGS(32, 36, 40, 44)
		FMTVR_FPU_REGS(vr4, vr5)
		LDW_FPU_REGS(48, 52, 56, 60)
		FMTVR_FPU_REGS(vr6, vr7)
		"addi   %4, 32\n"
		"addi   %4, 32\n"
		LDW_FPU_REGS(0, 4, 8, 12)
		FMTVR_FPU_REGS(vr8, vr9)
		LDW_FPU_REGS(16, 20, 24, 28)
		FMTVR_FPU_REGS(vr10, vr11)
		LDW_FPU_REGS(32, 36, 40, 44)
		FMTVR_FPU_REGS(vr12, vr13)
		LDW_FPU_REGS(48, 52, 56, 60)
		FMTVR_FPU_REGS(vr14, vr15)
		:"=a"(tmp1),"=a"(tmp2),"=a"(tmp3),
		"=a"(tmp4),"+a"(fpgr));
	local_irq_restore(flg);
out:
	return err;
}

