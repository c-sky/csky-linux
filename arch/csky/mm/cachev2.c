// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/spinlock.h>
#include <linux/smp.h>
#include <asm/cache.h>

#define SYNC asm volatile("sync.is\n")

void inline dcache_wb_line(unsigned long start)
{
	asm volatile("dcache.cval1 %0\n"::"r"(start));
	SYNC;
}

void icache_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (;i < end; i += L1_CACHE_BYTES)
		asm volatile("icache.iva %0\n"::"r"(i));
	SYNC;
}

void icache_inv_all(void)
{
	asm volatile("icache.ialls\n");
	SYNC;
}

void dcache_wb_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (;i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.cval1 %0\n"::"r"(i));
	SYNC;
}

void dcache_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (;i < end; i += L1_CACHE_BYTES) {
		asm volatile("dcache.cval1 %0\n"::"r"(i));
	}
	SYNC;
	for (;i < end; i += L1_CACHE_BYTES) {
		asm volatile("dcache.iva %0\n"::"r"(i));
	}
	SYNC;
}

void dcache_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (;i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.civa %0\n"::"r"(i));
	SYNC;
}

void cache_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (;i < end; i += L1_CACHE_BYTES) {
		asm volatile("dcache.cval1 %0\n"::"r"(i));
	}
	SYNC;
	for (;i < end; i += L1_CACHE_BYTES) {
		asm volatile("dcache.iva %0\n"::"r"(i));
	}
	SYNC;
	for (;i < end; i += L1_CACHE_BYTES) {
		asm volatile("icache.iva %0\n"::"r"(i));
	}
	SYNC;
}

void dma_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.civa %0\n"::"r"(i));
	SYNC;
}

void dma_wb_range(unsigned long start, unsigned long end)
{
	unsigned long i = start & ~(L1_CACHE_BYTES - 1);

	for (; i < end; i += L1_CACHE_BYTES)
		asm volatile("dcache.civa %0\n"::"r"(i));

	SYNC;
}
