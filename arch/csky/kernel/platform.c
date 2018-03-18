// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/init.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/sys_soc.h>
#include <linux/io.h>

static int __init csky_platform_init(void)
{
	return of_platform_default_populate(NULL, NULL, NULL);
}
device_initcall(csky_platform_init);


