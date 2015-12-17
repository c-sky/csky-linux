/*
 * linux/arch/csky/mm/l2cache.c
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

#ifdef CONFIG_CSKY_L2_CACHE_LINE_FLUSH /* CSKY_L2_CACHE_LINE_FLUSH */
/*
 * according to the parameters to flush/clear/invalid the 
 * all/data/instruction all.
 */
inline void
__flush_l2_all(unsigned long value){
	__asm__ __volatile__("mtcr %0,cr24\n\t"
	                     : :"r" (value));
}

/*
 * use ldw instruction to touch off the exception.
 */
#define ldw_addr(addr)					\
__asm__ __volatile__("ldw	%0, (%1, 0)\n\t"	\
			:"=r"(tmp) 			\
			:"r"(addr), "0"(tmp))	

#define get_cr24(value, readvalue)			\
__asm__ __volatile__("mtcr	%2, cr24\n\t"		\
			"mfcr	%0, cr24\n\t"		\
			"bclri	%1, %0, 31\n\t"		\
			"mtcr	%1, cr24\n\t"		\
			:"=r"(readvalue), "=r"(tmp)	\
			:"r"(value), "0"(readvalue), "1"(tmp))
							
#define set_cr24(value)					\
__asm__ __volatile__("mtcr	%0, cr24\n\t"		\
			::"r"(value))	
							
#define set_cr22(value)					\
__asm__ __volatile__("mtcr	%0, cr22\n\t"		\
			::"r"(value))	

/*
 * according to the parameters to flush/clear/invalid one line 
 * all/data/instruction cache.
 *
 * this function is using set and way to index in cr22.
 * cr24's ITS = 1
 */
void
__its_l2_flush_one_line(unsigned long set, unsigned long way, unsigned long value){
	unsigned long cr24value, cr22value;
	
	cr24value = CACHE_OMS | CACHE_ITS | value;
	cr22value = set << CR22_SET_SHIFT | way << CR22_WAY_SHIFT; 

	set_cr24(cr24value);
	set_cr22(cr22value);	
}

/*
 * according to the parameters to flush/clear/invalid the 
 * all/data/instruction during an range.
 *
 * this function is using virtual address to index in cr22.
 * cr24's ITS = 0
 */
#define MSIZE	PAGE_SIZE
void
__flush_l2_cache_range(unsigned long start, unsigned long end, unsigned long value){
	unsigned long i, len, cr24value, cr22value, readcr24value, tmp;

	len = end - start;

	if(unlikely(len >= MSIZE)){
		goto flush_all;
	}

	cr24value = L2_CACHE_OMS | value;
	cr22value = start;
	readcr24value = 0;
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
	get_cr24(cr24value, readcr24value);
	if(unlikely((readcr24value & L2_CACHE_LICF) != 0)){
		goto flush_all;
	}

	cr22value = end - 1;
	set_cr22(cr22value);
	get_cr24(cr24value, readcr24value);
	if(unlikely((readcr24value & L2_CACHE_LICF) != 0)){
		goto flush_all;
	}
#else
	set_cr22(cr22value);
	get_cr24(cr24value, readcr24value);
	if(unlikely((readcr24value & L2_CACHE_LICF) != 0)){
		ldw_addr(start);
	}

	cr22value = end - 1;
	set_cr22(cr22value);
	get_cr24(cr24value, readcr24value);
	if(unlikely((readcr24value & L2_CACHE_LICF) != 0)){
		ldw_addr(end - 1);	
	}
#endif /* MMU_HARD_REFILL */
	for(i = start; i < end; i += L2_CACHE_BYTES){
		set_cr22(i);
		set_cr24(cr24value);
	}

	return;	
flush_all:
      	__flush_l2_all(value);
}

#define disable_cr23(value)				\
__asm__ __volatile__("mfcr	%0, cr23\n\t"		\
			"bclri	%0, 3\n\t"		\
			"mtcr	%0, cr23\n\t"		\
			::"r"(value))	
void 
__flush_l2_disable(void){
	unsigned long value;

	disable_cr23(value);
}
#endif
