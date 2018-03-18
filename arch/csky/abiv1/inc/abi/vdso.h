// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/uaccess.h>

static inline int setup_vdso_page(unsigned short *ptr)
{
	int err;

	/* movi r1, 127 */
	err |= __put_user(0x6000 + (127 << 4)+1, ptr+0);
	/* addi r1, 32 */
	err |= __put_user(0x2000 + ((32-1)  << 4)+1, ptr+1);
	/* addi r1, 173-127-32 */
	err |= __put_user(0x2000 + (((173-127-32) -1)<< 4)+1, ptr+2);
	/* trap 0 */
	err |= __put_user(0x08, ptr+3);

	return err;
}
