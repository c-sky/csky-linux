// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/types.h>

void *memset(void *dest, int c, size_t l)
{
	char *d = dest;
	int ch = c;
	int tmp;

	if ((long)d & 0x3)
		while (l--) *d++ = ch;
	else {
		ch &= 0xff;
		tmp = (ch | ch << 8 | ch << 16 | ch << 24);

		while (l >= 16) {
			*(((long *)d)) = tmp;
			*(((long *)d)+1) = tmp;
			*(((long *)d)+2) = tmp;
			*(((long *)d)+3) = tmp;
			l -= 16;
			d += 16;
		}

		while (l > 3) {
			*(((long *)d)) = tmp;
			d = d + 4;
			l -= 4;
		}

		while (l) {
			*d++ = ch;
			l--;
		}
	}
	return dest;
}
