// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/syscalls.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/highuid.h>
#include <linux/personality.h>
#include <linux/tty.h>
#include <linux/binfmts.h>
#include <linux/tracehook.h>
#include <linux/freezer.h>
#include <linux/uaccess.h>

#include <asm/setup.h>
#include <asm/pgtable.h>
#include <asm/traps.h>
#include <asm/ucontext.h>
#include <asm/vdso.h>

#include <abi/regdef.h>
#ifdef CONFIG_CPU_HAS_FPU
#include <abi/fpu.h>
#endif

#define _BLOCKABLE (~(sigmask(SIGKILL) | sigmask(SIGSTOP)))

struct rt_sigframe
{
	int sig;
	struct siginfo *pinfo;
	void *puc;
	struct siginfo info;
	struct ucontext uc;
};

static inline int
restore_sigframe(struct pt_regs *regs, struct sigcontext *usc, int *pr2)
{
	int err = 0;
	int i = 0;
	unsigned long usp;

	/* Always make any pending restarted system calls return -EINTR */
	current_thread_info()->task->restart_block.fn = do_no_restart_syscall;

	/* restore passed registers */
	err |= __get_user(regs->a0, &usc->sc_a0);
	err |= __get_user(regs->a1, &usc->sc_a1);
	err |= __get_user(regs->a2, &usc->sc_a2);
	err |= __get_user(regs->a3, &usc->sc_a3);
	for(i = 0; i < 10; i++)
		err |= __get_user(regs->regs[i], &usc->sc_regs[i]);

	err |= __get_user(regs->r15, &usc->sc_r15);
#if defined(__CSKYABIV2__)
	for(i = 0; i < 16; i++)
	{
		err |= __get_user(regs->exregs[i], &usc->sc_exregs[i]);
	}
	err |= __get_user(regs->rhi, &usc->sc_rhi);
	err |= __get_user(regs->rlo, &usc->sc_rlo);
#endif
	err |= __get_user(regs->sr, &usc->sc_sr);
	err |= __get_user(regs->pc, &usc->sc_pc);
	err |= __get_user(usp, &usc->sc_usp);
	wrusp(usp);

#ifdef CONFIG_CPU_HAS_FPU
	err |= restore_fpu_state(usc);
#endif
	*pr2 = regs->a0;
	return err;
}

asmlinkage int
do_rt_sigreturn(void)
{
	unsigned long usp = rdusp();
	struct rt_sigframe *frame = (struct rt_sigframe *)usp;
	sigset_t set;
	int a0;
	struct pt_regs *regs = current_pt_regs();

	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	sigdelsetmask(&set, ~_BLOCKABLE);
	spin_lock_irq(&current->sighand->siglock);
	current->blocked = set;
	recalc_sigpending( );
	spin_unlock_irq(&current->sighand->siglock);

	if (restore_sigframe(regs, &frame->uc.uc_mcontext, &a0))
		goto badframe;

	return a0;

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}

/*
 * Set up a signal frame.
 */

static inline int setup_sigframe(struct sigcontext *sc, struct pt_regs *regs,
		unsigned long mask)
{
	int err = 0;
	int i = 0;

	err |= __put_user(mask, &sc->sc_mask);
	err |= __put_user(rdusp(), &sc->sc_usp);
	err |= __put_user(regs->a0, &sc->sc_a0);
	err |= __put_user(regs->a1, &sc->sc_a1);
	err |= __put_user(regs->a2, &sc->sc_a2);
	err |= __put_user(regs->a3, &sc->sc_a3);
	for(i = 0; i < 10; i++)
	{
		err |= __put_user(regs->regs[i], &sc->sc_regs[i]);
	}
	err |= __put_user(regs->r15, &sc->sc_r15);
#if defined(__CSKYABIV2__)
	for(i = 0; i < 16; i++)
	{
		err |= __put_user(regs->exregs[i], &sc->sc_exregs[i]);
	}
	err |= __put_user(regs->rhi, &sc->sc_rhi);
	err |= __put_user(regs->rlo, &sc->sc_rlo);
#endif
	err |= __put_user(regs->sr, &sc->sc_sr);
	err |= __put_user(regs->pc, &sc->sc_pc);

#ifdef CONFIG_CPU_HAS_FPU
	err |= save_fpu_state(sc, regs);
#endif

	return err;
}

static inline void *
get_sigframe(struct k_sigaction *ka, struct pt_regs *regs, size_t frame_size)
{
	unsigned long usp;

	/* Default to using normal stack.  */
	usp = rdusp();

	/* This is the X/Open sanctioned signal stack switching.  */
	if ((ka->sa.sa_flags & SA_ONSTACK) && !sas_ss_flags(usp)) {
		if (!on_sig_stack(usp))
			usp = current->sas_ss_sp + current->sas_ss_size;
	}
	return (void *)((usp - frame_size) & -8UL);
}

static int setup_rt_frame (int sig, struct k_sigaction *ka, siginfo_t *info,
		sigset_t *set, struct pt_regs *regs)
{
	struct rt_sigframe *frame;
	int err = 0;

	struct csky_vdso *vdso = current->mm->context.vdso;

	frame = get_sigframe(ka, regs, sizeof(*frame));
	if (!frame)
		return 1;

	err |= __put_user(sig, &frame->sig);
	err |= __put_user(&frame->info, &frame->pinfo);
	err |= __put_user(&frame->uc, &frame->puc);
	err |= copy_siginfo_to_user(&frame->info, info);

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __put_user((void *)current->sas_ss_sp,
			&frame->uc.uc_stack.ss_sp);
	err |= __put_user(sas_ss_flags(rdusp()),
			&frame->uc.uc_stack.ss_flags);
	err |= __put_user(current->sas_ss_size, &frame->uc.uc_stack.ss_size);
	err |= setup_sigframe(&frame->uc.uc_mcontext, regs, 0);
	err |= copy_to_user (&frame->uc.uc_sigmask, set, sizeof(*set));

	if (err)
		goto give_sigsegv;

	/* Set up registers for signal handler */
	wrusp ((unsigned long) frame);
	regs->pc = (unsigned long) ka->sa.sa_handler;
	regs->r15 = (unsigned long)vdso->rt_signal_retcode;

adjust_stack:
	regs->a0 = sig; /* first arg is signo */
	regs->a1 = (unsigned long)(&(frame->info)); /* second arg is (siginfo_t*) */
	regs->a2 = (unsigned long)(&(frame->uc));/* third arg pointer to ucontext */
	return err;

give_sigsegv:
	if (sig == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);
	goto adjust_stack;
}

/*
 * OK, we're invoking a handler
 */
static int
handle_signal(int sig, struct k_sigaction *ka, siginfo_t *info,
		sigset_t *oldset, struct pt_regs *regs)
{
	struct task_struct *tsk = current;
	int ret;

	/* set up the stack frame, regardless of SA_SIGINFO, and pass info anyway. */
	ret = setup_rt_frame(sig, ka, info, oldset, regs);

	if (ret != 0) {
		force_sigsegv(sig, tsk);
		return ret;
	}

	/* Block the signal if we were successful. */
	spin_lock_irq(&current->sighand->siglock);
	sigorsets(&current->blocked, &current->blocked, &ka->sa.sa_mask);
	if (!(ka->sa.sa_flags & SA_NODEFER))
		sigaddset(&current->blocked, sig);
	recalc_sigpending();
	spin_unlock_irq(&current->sighand->siglock);

	return 0;
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals
 * that the kernel can handle, and then we build all the user-level signal
 * handling stack-frames in one go after that.
 */
static void do_signal(struct pt_regs *regs, int syscall)
{
	unsigned int retval = 0, continue_addr = 0, restart_addr = 0;
	struct ksignal ksig;

	/*
	 * We want the common case to go fast, which
	 * is why we may in certain cases get here from
	 * kernel mode. Just return without doing anything
	 * if so.
	 */
	if (!user_mode(regs))
		return;

	current->thread.esp0 = (unsigned long)regs;

	/*
	 * If we were from a system call, check for system call restarting...
	 */
	if (syscall) {
		continue_addr = regs->pc;
#if defined(__CSKYABIV2__)
		restart_addr = continue_addr - 4;
#else
		restart_addr = continue_addr - 2;
#endif
		retval = regs->a0;

		/*
		 * Prepare for system call restart.  We do this here so that a
		 * debugger will see the already changed.
		 */
		switch (retval) {
		case -ERESTARTNOHAND:
		case -ERESTARTSYS:
		case -ERESTARTNOINTR:
			regs->a0 = regs->orig_a0;
			regs->pc = restart_addr;
			break;
		case -ERESTART_RESTARTBLOCK:
			regs->a0 = -EINTR;
			break;
		}
	}

	if (try_to_freeze())
		goto no_signal;

	/*
	 * Get the signal to deliver.  When running under ptrace, at this
	 * point the debugger may change all our registers ...
	 */
	if (get_signal(&ksig)) {
		sigset_t *oldset;

		/*
		 * Depending on the signal settings we may need to revert the
		 * decision to restart the system call.  But skip this if a
		 * debugger has chosen to restart at a different PC.
		 */
		if (regs->pc == restart_addr) {
			if (retval == -ERESTARTNOHAND
					|| (retval == -ERESTARTSYS
						&& !(ksig.ka.sa.sa_flags & SA_RESTART))) {
				regs->a0 = -EINTR;
				regs->pc = continue_addr;
			}
		}

		if (test_thread_flag(TIF_RESTORE_SIGMASK))
			oldset = &current->saved_sigmask;
		else
			oldset = &current->blocked;
		/* Whee!  Actually deliver the signal.  */
		if (handle_signal(ksig.sig, &ksig.ka, &ksig.info, oldset, regs) == 0) {
			/*
			 * A signal was successfully delivered; the saved
			 * sigmask will have been stored in the signal frame,
			 * and will be restored by sigreturn, so we can simply
			 * clear the TIF_RESTORE_SIGMASK flag.
			 */
			if (test_thread_flag(TIF_RESTORE_SIGMASK))
				clear_thread_flag(TIF_RESTORE_SIGMASK);
		}
		return;
	}

no_signal:
	if (syscall) {
		/*
		 * Handle restarting a different system call.  As above,
		 * if a debugger has chosen to restart at a different PC,
		 * ignore the restart.
		 */
		if (retval == -ERESTART_RESTARTBLOCK
				&& regs->pc == continue_addr) {
#if defined(__CSKYABIV2__)
			regs->regs[3] = __NR_restart_syscall;
			regs->pc -= 4;
#else
			regs->regs[9] = __NR_restart_syscall;
			regs->pc -= 2;
#endif
		}

		/* If there's no signal to deliver, we just put the saved sigmask
		 * back.
		 */
		if (test_thread_flag(TIF_RESTORE_SIGMASK)) {
			clear_thread_flag(TIF_RESTORE_SIGMASK);
			sigprocmask(SIG_SETMASK, &current->saved_sigmask, NULL);
		}
	}
}

asmlinkage void
do_notify_resume(unsigned int thread_flags, struct pt_regs *regs, int syscall)
{
	if (thread_flags & _TIF_SIGPENDING)
		do_signal(regs, syscall);

	if (thread_flags & _TIF_NOTIFY_RESUME) {
		clear_thread_flag(TIF_NOTIFY_RESUME);
		tracehook_notify_resume(regs);
	}
}
