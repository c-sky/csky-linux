// SPDX-License-Identifier: GPL-2.0
/*
 * Copied from arch/arm64/kernel/vdso/vgettimeofday.c
 *
 * Copyright (C) 2018 ARM Ltd.
 * Copyright (C) 2020 SiFive
 */

#include <linux/time.h>
#include <linux/types.h>

extern
int __vdso_gettimeofday(struct __kernel_old_timeval *tv, struct timezone *tz);
int __vdso_gettimeofday(struct __kernel_old_timeval *tv, struct timezone *tz)
{
	return __cvdso_gettimeofday(tv, tz);
}

#if __SIZEOF_POINTER__ == 8
extern
int __vdso_clock_gettime(clockid_t clock, struct __kernel_timespec *ts);
int __vdso_clock_gettime(clockid_t clock, struct __kernel_timespec *ts)
{
	return __cvdso_clock_gettime(clock, ts);
}

extern
int __vdso_clock_getres(clockid_t clock_id, struct __kernel_timespec *res);
int __vdso_clock_getres(clockid_t clock_id, struct __kernel_timespec *res)
{
	return __cvdso_clock_getres(clock_id, res);
}
#elif __SIZEOF_POINTER__ == 4
extern
int __vdso_clock_gettime(clockid_t clock, struct old_timespec32 *ts);
int __vdso_clock_gettime(clockid_t clock, struct old_timespec32 *ts)
{
	return __cvdso_clock_gettime32(clock, ts);
}

int __vdso_clock_gettime64(clockid_t clock, struct __kernel_timespec *ts);
int __vdso_clock_gettime64(clockid_t clock, struct __kernel_timespec *ts)
{
	return __cvdso_clock_gettime(clock, ts);
}

extern
int __vdso_clock_getres(clockid_t clock_id, struct old_timespec32 *res);
int __vdso_clock_getres(clockid_t clock_id, struct old_timespec32 *res)
{
	return __cvdso_clock_getres_time32(clock_id, res);
}

extern
int __vdso_clock_getres64(clockid_t clock_id, struct __kernel_timespec *res);
int __vdso_clock_getres64(clockid_t clock_id, struct __kernel_timespec *res)
{
	return __cvdso_clock_getres(clock_id, res);
}
#endif
