// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#define __ARCH_WANT_OLD_READDIR
#define __ARCH_WANT_RENAMEAT
#define __ARCH_WANT_STAT64
#define __ARCH_WANT_SYS_CLONE
#define __ARCH_WANT_SYS_FORK
#define __ARCH_WANT_SYS_GETHOSTNAME
#define __ARCH_WANT_SYS_IPC
#define __ARCH_WANT_SYS_LLSEEK
#define __ARCH_WANT_SYS_NICE
#define __ARCH_WANT_SYS_OLD_GETRLIMIT
#define __ARCH_WANT_SYS_SIGNAL
#define __ARCH_WANT_SYS_SIGPENDING
#define __ARCH_WANT_SYS_SIGPROCMASK
#define __ARCH_WANT_SYS_SOCKETCALL
#define __ARCH_WANT_SYS_VFORK
#define __ARCH_WANT_SYS_WAITPID

#undef	__NR_rt_sigreturn
#undef	__NR_getppid

#include <asm-generic/unistd.h>

/*
 * __NR_rt_sigreturn must be 173
 * Because gcc/config/csky/linux-unwind.h use hard-code 173
 * to parse rt_sigframe.
 * FIXME: Stupid copy from arm :( I'll rewrite the vdso.
 */
#if __NR_rt_sigreturn != 139
#error __NR_rt_sigreturn has changed.
#endif

#if __NR_getppid != 173
#error __NR_getppid has changed.
#endif

#undef	__NR_rt_sigreturn
#define	__NR_rt_sigreturn 173
__SC_COMP(__NR_rt_sigreturn, sys_rt_sigreturn, compat_sys_rt_sigreturn)

#undef	__NR_getppid
#define	__NR_getppid 139
__SYSCALL(__NR_getppid, sys_getppid)

/*
 * other define
 */
#define __NR_set_thread_area	(__NR_arch_specific_syscall + 0)
__SYSCALL(__NR_set_thread_area, sys_set_thread_area)
#define __NR_ipc		(__NR_arch_specific_syscall + 1)
__SYSCALL(__NR_ipc, sys_ipc)
#define __NR_socketcall		(__NR_arch_specific_syscall + 2)
__SYSCALL(__NR_socketcall, sys_socketcall)
#define __NR_ugetrlimit		(__NR_arch_specific_syscall + 3)
__SYSCALL(__NR_ugetrlimit, sys_getrlimit)
#define __NR_cacheflush		(__NR_arch_specific_syscall + 4)
__SYSCALL(__NR_cacheflush, sys_cacheflush)
#define __NR_sysfs		(__NR_arch_specific_syscall + 5)
__SYSCALL(__NR_sysfs, sys_sysfs)

__SYSCALL(__NR_fadvise64_64, sys_csky_fadvise64_64)

