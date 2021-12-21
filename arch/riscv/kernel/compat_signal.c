// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/compat.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/tracehook.h>
#include <linux/linkage.h>

#include <asm/ucontext.h>
#include <asm/vdso.h>
#include <asm/switch_to.h>
#include <asm/csr.h>

COMPAT_SYSCALL_DEFINE0(rt_sigreturn)
{
	return 0;
}
