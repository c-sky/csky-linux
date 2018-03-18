// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <asm/cache.h>

void inline dcache_wb_line(unsigned long start)
{
	asm volatile("dcache.cval1 %0\n"::"r"(start));
	asm volatile("sync.is\n");
}

void icache_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("icache.iva %0\n"::"r"(i));
	asm volatile("sync.is\n");
}

void icache_inv_all(void)
{
	asm volatile("icache.ialls\n");
	asm volatile("sync.is\n");
}

void dcache_wb_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.cval1 %0\n"::"r"(i));
	asm volatile("sync.is\n");
}

void dcache_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES) {
		asm volatile("dcache.cval1 %0\n"::"r"(i));
		asm volatile("dcache.iva %0\n"::"r"(i));
	}
	asm volatile("sync.is\n");
}

void dcache_inv_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.iva %0\n"::"r"(i));
	asm volatile("sync.is\n");
}

void dcache_wbinv_all(void)
{
	asm volatile("dcache.ciall\n");
	asm volatile("sync.is\n");
}

void cache_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES) {
		asm volatile("dcache.cval1 %0\n"::"r"(i));
		asm volatile("dcache.iva %0\n"::"r"(i));
		asm volatile("icache.iva %0\n"::"r"(i));
	}
	asm volatile("sync.is\n");
}

void cache_wbinv_all(void)
{
	asm volatile("dcache.ciall\n");
	asm volatile("icache.ialls\n");
	asm volatile("sync.is\n");
}

void dma_wbinv_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.civa %0\n"::"r"(i));
	asm volatile("sync.is\n");
}

void dma_wb_range(unsigned long start, unsigned long end)
{
	unsigned long i;
	for(i=start; i<end; i+=L1_CACHE_BYTES)
		asm volatile("dcache.cva %0\n"::"r"(i));
	asm volatile("sync.is\n");
}

