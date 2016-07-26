#ifndef __ASM_CSKY_SYSCALLS_H
#define __ASM_CSKY_SYSCALLS_H

#include <linux/linkage.h>

#include <asm-generic/syscalls.h>

/* kernel/sys_riscv.c */
asmlinkage long sys_sysriscv(unsigned long, unsigned long,
	unsigned long, unsigned long);

asmlinkage long sys_set_thread_area(unsigned long addr);

#endif /* __ASM_CSKY_SYSCALLS_H */
