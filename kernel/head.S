/*
 * hrad.S -- startup code for csky eval board.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file README.legal in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2009 Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2009 Hu junshan (junshan_hu@c-sky.com)
 *
 */

#include <linux/init.h>
#include <linux/threads.h>
#include <asm/setup.h>
#include <asm/csky.h>
#include <asm/page.h>
#include <asm/entry.h>
#include <asm/regdef.h>

/*
 *	Board setup info.
 */
#define	PHY_MEM_BASE	CK_RAM_BASE	/* Memory base at address 0 */

//Virtual address variable
.export _ramvec
.export swapper_pg_dir
.export empty_bad_page
.export invalid_pte_table
.export bootloader_tags

.export _start
.export _stext

.data

_ramvec:
.long	0

bootloader_tags:
.long	0

.align 12
swapper_pg_dir:
.fill	4096
empty_bad_page:
.fill	4096
invalid_pte_table:
.fill	4096

.align 10
virtual_vec_base:
.fill	128, 4

__HEAD
/*
 *	This is the codes first entry point. This is where it all
 *	begins...
 */
_stext:
_start:
	.rept   8
	mov     sp, sp
	.endr
	/* Check if the boot loader actually provided a tag table */
	lrw	    r4, ATAG_MAGIC
	cmpne   r4, a0
	bt      no_tag_table
	lrw     a0, bootloader_tags  /* Load address of bootloader_tags */
	bclri   a0, 31               /* convert to PA, because MMU disable */
	bseti   a1, 31               /* convert to VA */
	stw     a1, (a0)             /* Save the tag table address for later use */

no_tag_table:
	/*
	 * Init psr value, enable exception, disable interrupt and fast interrupt.
	 * psr = 0x80000100
	 */
	bgeni   r4, 31
	bseti   r4, 8
	mtcr    r4, psr

	/*
	 * Enable double data cache line.
	 * It must be enabled before cache on.
	 */
#ifdef CONFIG_CSKY_DOUBLE_DCACHE_LINE
	mfcr	r7, cr18
	bseti	r7, 18
	bclri	r7, 19
	mtcr	r7, cr18
#endif

	/*
	 *	Enable internal cache.
	 */
#ifdef CONFIG_CSKY_INSTRUCTION_CACHE
	//Invalid instruction cache
	movi    r7, 0x11
	mtcr    r7, cr17
	//Enable instruction cache
	mfcr    r7, cr18
	bseti   r7, 2
	mtcr    r7, cr18
#endif

#ifdef CONFIG_CSKY_DATA_CACHE
	//Invalid Data cache
	movi    r7, 0x12
	mtcr    r7, cr17
	//Enable data cache
	mfcr    r7, cr18
	bseti   r7, 3
	mtcr    r7, cr18
#endif

	mfcr	r7, cr17
	bseti   r7, 16   // invalid BHT
	bseti   r7, 17   // invalid BTB
	mtcr    r7, cr17

#ifdef CONFIG_CSKY_L2_CACHE
	// Invalid L2 cache
	movi    r7, 0x10
	mtcr    r7, cr24
	// Enable data cache
	mfcr    r7, cr23
	bseti   r7, 3
	// write back L2
	bseti   r7, 4
	// write alloc
	bseti   r7, 12
	mtcr    r7, cr23
#endif

/*
 * Setup the initial page tables.  We only setup the barest
 * amount which are required to get the kernel running, which
 * generally means mapping in the kernel code.
 *
 * We only map in 2 * PAGE_SIZE of RAM, which should be sufficient in
 * all cases.
 *
 * r5, r6 = physical address of start RAM
 * r7 = Virtual address
 */
__create_page_table:
	/*
	 * Create identity mapping for first MB of kernel to
	 * cater for the MMU enable.  This identity mapping
	 * will be removed by paging_init()
	 */

	lrw     r8, PHY_MEM_BASE  //get physical address of start RAM   
	movi    a0, 0
#ifdef CONFIG_CPU_CSKYV1
	movi    r5, 0x00000016 //C = 010, D = 1, V = 1, G = 0, pfn = 0x0
	movi    r6, 0x00000056 //C = 010, D = 1, V = 1, G = 0, pfn = 0x1
#else
	movi    r5, 0x00000006 //C = 0, D = 1, V = 1, pfn = 0x0
	movi    r6, 0x00001006 //C = 0, D = 1, V = 1, pfn = 0x1
#endif
	movi    r7, 0x00000000 //VPN2 = 0x0, ASID = 0
	bmaski  r9, 13
	andn    r8, r9
	or      r7, r8          // VPN2 is bit 31-12 of phy addr
#ifdef CONFIG_CPU_CSKYV1
	lsri    r8, 6	        // get bit 31-12 of phy addr, only MMUv1 need.
#endif
	or      r5, r8		    //pfn	
	or      r6, r8          //pfn

	//invalid mmu jtlb(ASID)
	SET_CP_MMU
	bgeni   r4, 26
	WR_MCIR r4

	WR_MPR	a0              // page mask = 4k
	WR_MEL0	r5
	WR_MEL1	r6
	WR_MEH	r7

	//Initial TLB
	bgeni   a0, 28	 	    //TLBWR
	WR_MCIR	a0

#ifdef CONFIG_PHYSICAL_BASE_CHANGE
	lrw     a0, CONFIG_SSEG0_BASE
	movi    a1, 0xe                // C = 1, D = 1, V = 1
	or      a0, a1
	WR_MSA0 a0
	lrw     a0, CONFIG_SSEG1_BASE
	movi    a1, 0x6                // D = 1, V = 1
	or      a0, a1
	WR_MSA1 a0
#else
#ifdef CONFIG_CPU_CSKYV1
	//movi	a0, 1
	//WR_MSA1	a0
#endif
#endif
	/*
	 *  Enable  CPU MMU.
	 */
	mfcr    r7, cr18
	bseti   r7, 0
	bclri   r7, 1
	mtcr    r7, cr18
	
	/*
	 * Enable predict for jump, return stack, and cache write back.
	 */
	mfcr    r7 , cr18
	bseti   r7 , 11                 /* BTB */
	bseti   r7 , 6                  /* branch prediction */
	bseti   r7 , 5                  /* forecast jump  for ck610  */
	bseti   r7 , 4                  /* write back for ck610  */
	mtcr    r7 , cr18 
	
	jmpi	__create_vector_table   /* P2V, jump from physic address to virtual
										 address */

	/*
	 *	Setup initial vector base table for interrupts and exceptions
	 */
__create_vector_table:
	lrw     r4, virtual_vec_base    /* Load pre-defined vbr */
	lrw     a0, _ramvec             /* Load address of _ramvec */
	stw     r4, (a0)                /* Write vbr setting to it */
	mtcr    r4, vbr                 /* Set vbr register with physical address */

	/*
	 *	Zero the bss region.
	 */
	lrw	    r4, _sbss               /* Get start of bss */
	lrw	    a0, _ebss               /* Get end of bss */
	subu    a0, r4                  /* Calculate size of bss */
	lsri    a0, 2                   /* Size of whole words */

	movi    a1, 0                   /* Set zero value to write */
       
1:
	stw     a1, (r4)                /* Zero next word */
	addi    r4, 4                   /* Increase bss pointer */
	//decne   a0                      /* Decrease counter */
 	subi    a0, 1
        cmpnei  a0, 0
	bt      1b                      /* Repeat for all bss */

	/*
	 *	Load the current task stack.
	 */
	lrw     r4, init_thread_union   /* Get address of init_thread_union */
	lrw     a0, THREAD_SIZE         /* 8K memory, 2 page for stack */
	addu     r4, a0                  /* Add 2 page offset to r4: 
	                                          task struct pointer */
	mov     sp, r4                  /* Set current task stack pointer */

	/*
	 *	Assember start up done, start code proper.
	 */
	jbsr	start_kernel			/* Start Linux kernel */

1:
	br      1b				        /* Should never get here */

