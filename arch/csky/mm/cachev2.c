// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/spinlock.h>
#include <asm/cache.h>

#define SYNC asm volatile("sync.is\n")

void inline dcache_wb_line(unsigned long start)
{
	asm volatile("dcache.cval1 %0\n"::"r"(start));
	SYNC;
}

int inline check_out_of_range(unsigned long start, unsigned long end)
{
	if (unlikely(start <  PAGE_OFFSET) ||
	    unlikely(start >= PAGE_OFFSET + LOWMEM_LIMIT))
		return 1;
	else
		return 0;
}

void icache_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i;

	if (check_out_of_range(start, end)) {
		icache_inv_all();
		return;
	}

	for(i=start; i<end; i+=L1_CACHE_BYTES)
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
	unsigned long i;

	if (check_out_of_range(start, end)) {
		asm volatile("dcache.call\n");
		SYNC;
		return;
	}

	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.cval1 %0\n"::"r"(i));
	SYNC;
}

void dcache_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i;

	if (check_out_of_range(start, end)) {
		dcache_wbinv_all();
		return;
	}

	for(i=start; i<end; i+=L1_CACHE_BYTES) {
		asm volatile("dcache.cval1 %0\n"::"r"(i));
		asm volatile("dcache.iva %0\n"::"r"(i));
	}
	SYNC;
}

void dcache_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i;

	if (check_out_of_range(start, end)) {
		dcache_wbinv_all();
		return;
	}

	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.iva %0\n"::"r"(i));
	SYNC;
}

void dcache_wbinv_all(void)
{
	asm volatile("dcache.ciall\n");
	SYNC;
}

void cache_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i;

	if (check_out_of_range(start, end)) {
		cache_wbinv_all();
		return;
	}

	for(i=start; i<end; i+=L1_CACHE_BYTES) {
		asm volatile("dcache.cval1 %0\n"::"r"(i));
		asm volatile("dcache.iva %0\n"::"r"(i));
		asm volatile("icache.iva %0\n"::"r"(i));
	}
	SYNC;
}

void cache_wbinv_all(void)
{
	asm volatile("dcache.ciall\n");
	asm volatile("icache.ialls\n");
	SYNC;
}

void dma_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.civa %0\n"::"r"(i));
	SYNC;
}

void dma_wb_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.cva %0\n"::"r"(i));
	SYNC;
}

