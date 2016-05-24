#include <linux/sched.h>

asmlinkage void sys_fork(void)
{
	do_fork(SIGCHLD, rdusp(), 0,NULL,NULL);
}

asmlinkage void sys_vfork(void)
{
	do_fork(CLONE_VFORK | CLONE_VM | SIGCHLD,
		rdusp(), 0,NULL,NULL);
}

asmlinkage void sys_clone(
	unsigned long clone_flags,
	unsigned long newsp,
	int __user *parent_tidptr,
	int __user *child_tidptr,
	int tls_val)
{
	if (!newsp) newsp = rdusp();

	do_fork(clone_flags, newsp, 0,
		parent_tidptr, child_tidptr);
}

asmlinkage void sys_set_thread_area(unsigned long addr)
{
	struct thread_info *ti = task_thread_info(current);

#if defined(CONFIG_CPU_CSKYV2)
	struct pt_regs *reg = current_pt_regs();
	reg->exregs[15] = (long)addr;
#endif
	ti->tp_value = addr;
}

