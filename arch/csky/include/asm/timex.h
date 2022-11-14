/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _ASM_CSKY_TIMEX_H
#define _ASM_CSKY_TIMEX_H

typedef unsigned long cycles_t;

static inline cycles_t get_cycles(void)
{
	cycles_t tmp;
	__asm__ __volatile(
		"cprgr %0, <0, 0x2>\n"
		: "=r"(tmp)
		:
		: "memory");
	return tmp;
}
#define get_cycles get_cycles

extern void time_init(void);

#endif /* _ASM_CSKY_TIMEX_H */
