// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/types.h>

/*
 * memory copy function.
 */
void *memcpy (void *to, const void *from, size_t l)
{
	char *d = to;
	const char *s = from;

	if (((long)d | (long)s) & 0x3)
	{
		while (l--) *d++ = *s++;
	}
	else
	{
        	while (l >= 16)
		{
			*(((long *)d)) = *(((long *)s));
			*(((long *)d)+1) = *(((long *)s)+1);
			*(((long *)d)+2) = *(((long *)s)+2);
			*(((long *)d)+3) = *(((long *)s)+3);
			l -= 16;
			d += 16;
			s += 16;
		}
		while (l > 3)
		{
			*(((long *)d)) = *(((long *)s));
			d = d +4;
			s = s +4;
			l -= 4;
		}
		while (l)
		{
			*d++ = *s++;
			l--;	
		}
	}
	return to;
}
