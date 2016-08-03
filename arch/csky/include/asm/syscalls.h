#ifndef __ASM_CSKY_SYSCALLS_H
#define __ASM_CSKY_SYSCALLS_H

#include <linux/linkage.h>

#include <asm-generic/syscalls.h>

asmlinkage long sys_cacheflush(void __user *, unsigned long, int);

asmlinkage long sys_set_thread_area(unsigned long addr);

#endif /* __ASM_CSKY_SYSCALLS_H */
