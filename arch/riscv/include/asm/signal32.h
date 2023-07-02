/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_SIGNAL32_H
#define __ASM_SIGNAL32_H

#if IS_ENABLED(CONFIG_COMPAT) || IS_ENABLED(CONFIG_ARCH_RV64ILP32)
int compat_setup_rt_frame(struct ksignal *ksig, sigset_t *set,
			  struct pt_regs *regs);
long __riscv_compat_rt_sigreturn(void);
#else
static inline
int compat_setup_rt_frame(struct ksignal *ksig, sigset_t *set,
			  struct pt_regs *regs)
{
	return -1;
}

static inline
long __riscv_compat_rt_sigreturn(void)
{
	return -1;
}
#endif

void __riscv_rt_sigreturn_badframe(void);

#endif
