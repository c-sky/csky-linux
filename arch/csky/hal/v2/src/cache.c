/*
 * linux/arck/csky/mm/cachev2.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006 Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Ye Yun (yun_ye@c-sky.com)
 */

#include <linux/mm.h>
#include <linux/sched.h>
#include <asm/cache.h>
#include <asm/cacheflush.h>

void _flush_cache_all(void)
{
	int value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void ___flush_cache_all(void)
{
	int value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_cache_mm(struct mm_struct *mm)
{
	int value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_cache_page(struct vm_area_struct *vma, unsigned long page)
{
	int value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	int value = 0x11;
	
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_cache_range(struct vm_area_struct *mm, unsigned long start, unsigned long end)
{
	int value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_cache_sigtramp(unsigned long addr)
{
	int value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_icache_all(void)
{
	int value = 0x11;
	
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void _flush_dcache_page(struct page * page)
{ 
        int value = 0x32;
	
        __asm__ __volatile__("mtcr %0,cr17\n\t"
                             : :"r" (value));
}

void _flush_dcache_all(void)
{ 
        int value = 0x32;
	
        __asm__ __volatile__("mtcr %0,cr17\n\t"
                             : :"r" (value));
}

void _flush_icache_range(unsigned long start, unsigned long end)
{
	int value = 0x11;
	
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     : :"r" (value));
}

void
_clear_dcache_all(void)
{
        int value = 0x22;
	
        __asm__ __volatile__("mtcr %0,cr17\n\t"
                             : :"r" (value));
}

void _clear_dcache_range(unsigned long start, unsigned long end)
{
	int value = 0x22;

	__asm__ __volatile__("mtcr %0,cr17\n\t"
	                     : :"r" (value));
}

#ifdef CONFIG_CSKY_CACHE_LINE_FLUSH /* CSKY_CACHE_LINE_FLUSH */
/*
 * according to the parameters to flush/clear/invalid the 
 * all/data/instruction all.
 */
static inline void
__flush_all(unsigned long value){
	__asm__ __volatile__("mtcr %0,cr17\n\t"
	                     : :"r" (value));
}

/*
 * use ldw instruction to touch off the exception.
 */
#define ldw_addr(addr)					\
__asm__ __volatile__("ldw	%0, (%1, 0)\n\t"	\
			:"=r"(tmp) 			\
			:"r"(addr), "0"(tmp))	

#define get_cr17(value, readvalue)			\
__asm__ __volatile__("mtcr	%2, cr17\n\t"		\
			"mfcr	%0, cr17\n\t"		\
			"bclri	%1, %0, 31\n\t"		\
			"mtcr	%1, cr17\n\t"		\
			:"=r"(readvalue), "=r"(tmp)	\
			:"r"(value), "0"(readvalue), "1"(tmp))
							
#define set_cr17(value)					\
__asm__ __volatile__("mtcr	%0, cr17\n\t"		\
			::"r"(value))	
							
#define set_cr22(value)					\
__asm__ __volatile__("mtcr	%0, cr22\n\t"		\
			::"r"(value))	

/*
 * according to the parameters to flush/clear/invalid one line 
 * all/data/instruction cache.
 *
 * this function is using set and way to index in cr22.
 * cr17's ITS = 1
 */
void
__its_flush_one_line(unsigned long set, unsigned long way, unsigned long value){
	unsigned long cr17value, cr22value, level;
	
	level = 1;
	cr17value = CACHE_OMS | CACHE_ITS | value;
	cr22value = set << CR22_SET_SHIFT | way << CR22_WAY_SHIFT \
			| level << CR22_LEVEL_SHIFT; 

	set_cr17(cr17value);
	set_cr22(cr22value);	
}

/*
 * according to the parameters to flush/clear/invalid the 
 * all/data/instruction during an range.
 *
 * this function is using virtual address to index in cr22.
 * cr17's ITS = 0
 */
#define MSIZE	PAGE_SIZE
void
__flush_cache_range(unsigned long start, unsigned long end, unsigned long value){
	unsigned long i, len, cr17value, cr22value, readcr17value, tmp;

	len = end - start;

	if(unlikely(len >= MSIZE)){
		goto flush_all;
	}

	cr17value = CACHE_OMS | value;
	cr22value = start;
	readcr17value = 0;
	tmp = 0;

	/* 
	 * touch off the exception.
	 *
	 * the range may across two page, and less than 4K.
	 * so rather than determine whether across or not,
	 * justing test start and end address will faster.
	 */
#ifdef CONFIG_MMU_HARD_REFILL
	set_cr22(cr22value);
	get_cr17(cr17value, readcr17value);
	if(unlikely((readcr17value & CACHE_LICF) != 0)){
		goto flush_all;
	}

	cr22value = end - 1;
	set_cr22(cr22value);
	get_cr17(cr17value, readcr17value);
	if(unlikely((readcr17value & CACHE_LICF) != 0)){
		goto flush_all;
	}
#else
	set_cr22(cr22value);
	get_cr17(cr17value, readcr17value);
	if(unlikely((readcr17value & CACHE_LICF) != 0)){
		ldw_addr(start);
	}

	cr22value = end - 1;
	set_cr22(cr22value);
	get_cr17(cr17value, readcr17value);
	if(unlikely((readcr17value & CACHE_LICF) != 0)){
		ldw_addr(end - 1);	
	}
#endif /* MMU_HARD_REFILL */
	for(i = start; i < end; i += L1_CACHE_BYTES){
		set_cr22(i);
		set_cr17(cr17value);
	}

	return;	
flush_all:
      	__flush_all(value);
}

#endif
