#include <linux/syscalls.h>
#include <asm/syscalls.h>

#undef __SYSCALL
#define __SYSCALL(nr, call) [nr] = (call),

void * const sys_call_table[__NR_syscalls] __page_aligned_data = {
	[0 ... __NR_syscalls - 1] = sys_ni_syscall,
#include <asm/unistd.h>
};
