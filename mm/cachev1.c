/*
 * linux/arck/csky/mm/ckcache.c
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
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void ___flush_cache_all(void)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_cache_mm(struct mm_struct *mm)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_cache_page(struct vm_area_struct *vma, unsigned long page)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x11;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_cache_range(struct vm_area_struct *mm, unsigned long start, unsigned long end)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_cache_sigtramp(unsigned long addr)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x33;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_icache_all(void)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x11;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _flush_dcache_page(struct page * page)
{ 
        int value = 0x32;
	
        __asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
                             : :"r" (value));
}

void _flush_dcache_all(void)
{ 
        int value = 0x32;
	
        __asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
                             : :"r" (value));
}

void _flush_icache_range(unsigned long start, unsigned long end)
{
	int value = 0;
	unsigned long flags;

	local_irq_save(flags);
	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bclri %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));

	value = 0x11;
	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
			     : :"r" (value));

	__asm__ __volatile__("mfcr %0, cr18\n\t"
			     "bseti %0, 2\n\t"
			     "mtcr %0, cr18\n\t"
			     : :"r" (value));
	local_irq_restore(flags);
}

void _clear_dcache_all(void)
{
        int value = 0x22;
	
        __asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
                             : :"r" (value));
}

void _clear_dcache_range(unsigned long start, unsigned long end)
{
	int value = 0x22;

	__asm__ __volatile__("mtcr %0,cr17\n\t"
			     "sync\n\t"
	                     : :"r" (value));
}

#ifdef CONFIG_CSKY_CACHE_LINE_FLUSH /* CSKY_CACHE_LINE_FLUSH */
/* 
 * reserve a two dimensional array for flush cache line.
 */
static char __flush_retention[CSKY_CACHE_SIZE / L1_CACHE_BYTES] \
		[L1_CACHE_BYTES]__attribute__((aligned(CSKY_CACHE_SIZE)));

#define RES_START	((unsigned long)__flush_retention) 
#define WAY_SIZE_MASK	(CSKY_CACHE_WAY_SIZE - 1)
#define LINE_LEN_MASK	(L1_CACHE_BYTES - 1)
#define ADDR_MASK	(WAY_SIZE_MASK & (~LINE_LEN_MASK))

static int array_have_init = 0;

/* 
 * initialization reserve array for flush instruction cache, devide it into two 
 * halves, the first one initialized by jmp r2, the other one used jmp r15.
 * then access the address which is alignment as 8K to complete it.
 */
static inline void
__cache_fluch_init(void){
	unsigned long i, hindex;

	hindex = CSKY_CACHE_SIZE / L1_CACHE_BYTES / 2;
	for(i = 0; i < hindex ; i++){
		__flush_retention[i][0] = JMP_R2;
		__flush_retention[i + hindex][0] = JMP_R15;
	}
}

#define ldw_range(start, end)				\
__asm__ __volatile__("1:\n\t"				\
			"ldb	%0, (%1, 0)\n\t"	\
			"addi	%1, 16\n\t"		\
			"cmphs	%1, %2\n\t"		\
			"bf	1b\n\t"			\
			:"=r"(tmp) 			\
			:"r" (start), "r"(end), "0"(tmp));	
	
/* 
 * while flushing data cache, we can read specific position to flush out one 
 * specified cache line.
 */
void 
__flush_dcache_range(unsigned long start, unsigned long end){
/* assume CK610 is 2 ways */
#if CSKY_CACHE_WAY == 2
	unsigned long len, startaddr, endaddr;
	unsigned long macrostart, macroend, tmp;

	len = end - start;
	if(likely(len < CSKY_CACHE_WAY_SIZE)){
		startaddr = (start & WAY_SIZE_MASK) | RES_START; 
		endaddr = ((end & WAY_SIZE_MASK) | RES_START) + 16;

		if(startaddr > endaddr){
			macrostart = RES_START;
			macroend = endaddr;
			ldw_range(macrostart, macroend);
						
			macrostart = startaddr;
			macroend = endaddr + CSKY_CACHE_WAY_SIZE;
			ldw_range(macrostart, macroend);
						
			macrostart = startaddr + CSKY_CACHE_WAY_SIZE;
			macroend = RES_START + CSKY_CACHE_SIZE;
			ldw_range(macrostart, macroend);
		}else{
			macrostart = startaddr;
			macroend = endaddr;
			ldw_range(macrostart, macroend);

			macrostart = startaddr + CSKY_CACHE_WAY_SIZE;
			macroend = endaddr + CSKY_CACHE_WAY_SIZE;
			ldw_range(macrostart, macroend);		
		}
	}else{
		_flush_dcache_all();
	}
#else
#error "ck610 cache line flush not support 4 way yet..."
#endif
}

/* if we need use jsr we will destory the r15 */
#define jsr_range(start, end)				\
__asm__ __volatile__("mov	r2, %0\n\t"		\
			"1:\n\t"			\
			"jsr	%1\n\t"			\
			"addi	r2, 16\n\t"		\
			"addi	%1, 16\n\t"		\
			"cmphs	%1, %2\n\t"		\
			"bf	1b\n\t"			\
			:"=r"(r2)			\
			:"r" (start), "r"(end), "0"(r2)	\
			:"r2","r15");

/*
 * To fulsh the instruction cache, we need execute two instructions 
 * which's last 13 bits is same as the address. so here we use the jsr
 * and jmp r15 instruction.
 */
void 
__flush_icache_range(unsigned long start, unsigned long end)
{
/* assume CK610 is 2 ways */
#if CSKY_CACHE_WAY == 2
	unsigned long len, r2, needend;
	unsigned long startaddr, endaddr, macrostart, macroend;

	/* 
	 * in order to use for flush instruction cache, 
	 * we need to init the retention area. 
	 */
	if(array_have_init == 0){
		__cache_fluch_init();	
		array_have_init = 1;
	}
	
	len = end - start;
	if(likely(len < CSKY_CACHE_WAY_SIZE)){
		startaddr = (start & ADDR_MASK) | RES_START;
		needend = (end + LINE_LEN_MASK) & (~LINE_LEN_MASK);
		endaddr = (needend & ADDR_MASK) | RES_START;
		r2 = startaddr + CSKY_CACHE_WAY_SIZE;

		if(startaddr > endaddr){
			macrostart = RES_START;
			macroend = endaddr;
			jsr_range(macrostart, macroend);
						
			macrostart = startaddr;
			macroend = RES_START + CSKY_CACHE_WAY_SIZE;
			jsr_range(macrostart, macroend);
		}else{
			macrostart = startaddr;
			macroend = endaddr;
			jsr_range(macrostart, macroend);
		}					
	}else{
		_flush_icache_all();
	}
#else
#error "ck610 cache line flush not support 4 way yet..."
#endif
}

/*
 * there is not length check, so please make sure the arguements 
 * end minus start is less than 8K.
 */
void 
__flush_all_range(unsigned long start, unsigned long end){
	__flush_icache_range(start, end);
	__flush_dcache_range(start, end);
}

#endif
