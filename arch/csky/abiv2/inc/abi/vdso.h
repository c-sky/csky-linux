// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/uaccess.h>

static inline int setup_vdso_page(unsigned short *ptr)
{
	int err;

	/* movi r7, 173 */
	err |= __put_user(0xEA00+7, ptr);
	err |= __put_user(173,      ptr+1);

	/* trap 0 */
	err |= __put_user(0xC000,   ptr+2);
	err |= __put_user(0x2020,   ptr+3);

	return err;
}
