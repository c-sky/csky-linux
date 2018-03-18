// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/kernel.h>
#include <linux/oprofile.h>
#include <linux/errno.h>
#include <linux/init.h>

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	return oprofile_perf_init(ops);
}

void oprofile_arch_exit(void)
{
	oprofile_perf_exit();
}
