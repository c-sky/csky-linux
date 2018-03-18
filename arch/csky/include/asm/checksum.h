// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#ifndef __ASM_CSKY_CHECKSUM_H
#define __ASM_CSKY_CHECKSUM_H

#include <linux/in6.h>
#include <asm/byteorder.h>

static inline __sum16 csum_fold(__wsum csum)
{
	u32 tmp;
	asm volatile(
		"mov	%1, %0\n"
		"rori	%0, 16\n"
		"addu	%0, %1\n"
		"lsri	%0, 16\n"
		:"=r"(csum), "=r"(tmp)
		:"0"(csum));
	return (__force __sum16)~csum;
}
#define csum_fold csum_fold

static inline __wsum
csum_tcpudp_nofold(
	__be32 saddr,
	__be32 daddr,
	unsigned short len,
	unsigned short proto,
	__wsum sum
	)
{
	asm volatile(
		"clrc\n"
		"addc    %0, %1\n"
		"addc    %0, %2\n"
		"addc    %0, %3\n"
		"inct    %0\n"
		:"=r"(sum)
		:"r"((__force u32)saddr),
		"r"((__force u32)daddr),
#ifdef __BIG_ENDIAN
		"r"(proto + len),
#else
		"r"((proto + len) << 8),
#endif
		"0" ((__force unsigned long)sum)
		:"cc");
	return sum;
}
#define csum_tcpudp_nofold csum_tcpudp_nofold

static __inline__ __sum16
csum_ipv6_magic(
	const struct in6_addr *saddr,
	const struct in6_addr *daddr,
	__u32 len,
	unsigned short proto,
	__wsum sum
	)
{
	sum += saddr->in6_u.u6_addr32[0];
	sum += saddr->in6_u.u6_addr32[1];
	sum += saddr->in6_u.u6_addr32[2];
	sum += saddr->in6_u.u6_addr32[3];
	sum += daddr->in6_u.u6_addr32[0];
	sum += daddr->in6_u.u6_addr32[1];
	sum += daddr->in6_u.u6_addr32[2];
	sum += daddr->in6_u.u6_addr32[3];
	sum += (len + proto);

	return csum_fold(sum);
}
#define _HAVE_ARCH_IPV6_CSUM

#include <asm-generic/checksum.h>

#endif /* __ASM_CSKY_CHECKSUM_H */
