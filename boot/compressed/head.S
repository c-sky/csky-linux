#include <asm/csky.h>
#include <asm/entry.h>
/*
 *	Board setup info.
 */
.data
def_vec_base:
.rept 128
.long __default_exception_handler
.endr

/*
 *	This is the codes first entry point. This is where it all
 *	begins...
 */
	.section .start,"ax"
	.align
_stext:
_start:
	.rept   8
	mov     sp,sp
	.endr
	mtcr    r2, ss2
	mtcr    r3, ss3
	/*
	 * Init psr value, enable exception, disable interrupt and fast interrupt.
	 * psr = 0x80000100
	 */
	bgeni   r1, 31
	bseti   r1, 8
	mtcr    r1, psr
	
	/*
	 *      Enable internal cache.
	 */
	//Invalid instruction cache
	movi    r7, 0x11
	mtcr    r7, cr17
	//Enable instruction cache
	mfcr    r7, cr18
	bseti   r7, 2
	mtcr    r7, cr18
	//Invalid Data cache
	movi    r7, 0x12
	mtcr    r7, cr17
	//Enable data cache
	mfcr    r7, cr18
	bseti   r7, 3
	mtcr    r7, cr18

#ifdef CONFIG_MMU
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
	
	lrw     r3, operaddr  //get physical address of start RAM
	bmaski	r2, 12
	lsli    r2, 13
	movi    r5, 0x0000001e //C = 011, D = 1, V = 1, G = 0, pfn = 0x0
	movi    r6, 0x0 	//C = 010, D = 1, V = 1, G = 0, pfn = 0x1
	movi    r7, 0x00000000 //VPN2 = 0x0, ASID = 0
	bmaski  r4, 13
	andn    r3, r4
	or      r7, r3          // VPN2 is bit 31-12 of phy addr
#ifdef CONFIG_MMU_V1
	lsri    r3, 6           // get bit 31-12 of phy addr
#endif
	or      r5, r3          //pfn
	and     r6, r3          //pfn
	
	SET_CP_MMU
	
	//invalid mmu jtlb(ASID)
	bgeni   r1, 26
	WR_MCIR r1
	
	WR_MPR  r2              // page mask = 4k
	WR_MEL0 r5
	WR_MEL1 r6
	WR_MEH  r7
	
	//Initial TLB
	bgeni   r2, 28              //TLBWR
	WR_MCIR r2
	
	/*
	 * Enable predict for jump, return stack, and cache write back.
	 */
	mfcr    r7 , cr18
	bseti   r7 , 11                 /* BTB */
	bseti   r7 , 6                  /* branch prediction */
	bseti   r7 , 5                  /* forecast jump  for ck610  */
	bseti   r7 , 4                  /* write back for ck610  */
	mtcr    r7 , cr18
#else      /* CONFIG_MMU */
	/* 
	 * Setup MGU.
	 * priority setting, ff -- read and write
	 */
	lrw     r7, 0xff02
	mtcr    r7, cr19
	
	/* The first block */
	movi    r10, 0
	mtcr    r10, cr21
	lrw     r7, 0x3f /*baseaddr: 0x0; size: 4G*/
	mtcr    r7, cr20
	
	/* The second block */
	movi    r10, 1
	mtcr    r10, cr21
	lrw     r7, 0x3f /*baseaddr: 0x0; size: 4G*/
	mtcr    r7, cr20
#endif  /* CONFIG_MMU */

	/*
     *  Enable CPU MMU or MGU.
	 */
	mfcr    r7, cr18
	bseti   r7, 0
	bclri   r7, 1
	mtcr    r7, cr18

	/*
	 *	Setup initial vector base table for interrupts and exceptions
	 */
__create_vector_table:
	lrw     r1, def_vec_base    /* Load pre-defined vbr */
	mtcr    r1, vbr                 /* Set vbr register with physical address */

	lrw     r1, LC0
	lrw     r4, zreladdr
	lrw     r5, _start
	lrw     r6, _got_start
	lrw     r7, _got_end
	lrw     r10, user_stack+4096
	mov     sp,r10
#ifdef CONFIG_CPU_CSKYV1
	mov     r10,r1
#else
	grs     r10, LC0
#endif
	cmpne   r10,r1
	sub     r10, r10, r1		// calculate the delta offset
					// if delta is zero, we are
	bf	not_relocated		// running at the address we
					// were linked at.

	/*
	 * We're running at a different address.  We need to fix
	 * up various pointers:
	 *   r5 - zImage base address
	 *   r6 - GOT start
	 *   r7 - GOT end
	 *"/"
	add     r5, r10
	add     r6, r10
	add     r7, r10

	"/"*
	 * Relocate entries in the GOT table.  We only relocate
	 * the entries that are outside the (relocated) BSS region.
	 *"/"
1:	ldm	r1, (r6)		// relocate entries in the GOT
	cmp	r1, r2			// entry < bss_start ||
	cmphs	r3, r1			// _end < entry
	addlo	r1, r10			// table.  This fixes up the
	stw	r1, (r6,0)		// C references.
	addi 	r6,4
	cmplt	r6, r7
	bt	1b
	*/

not_relocated:
	/*
	 *	Zero the bss region.
	 */
	lrw     r1, _sbss               /* Get start of bss */
	lrw     r2, _ebss               /* Get end of bss */
	subu    r2, r1                  /* Calculate size of bss */
	lsri    r2, 2                   /* Size of whole words */
	movi    r3, 0                   /* Set zero value to write */
      
1:
	stw     r3, (r1)                /* Zero next word */
	addi    r1, 4                   /* Increase bss pointer */
	decne   r2                      /* Decrease counter */
	bt      1b                      /* Repeat for all bss */

	lrw     r4, zreladdr
	lrw     r5,_start
	lrw     r2,user_stack+4096
	mov     sp, r2

	mov     r1, sp			// ma1loc space above stack
	lrw     r2, 0x10000
	add     r2, sp	// 64k max


/*
 * Check to see if we will overwrite ourselves.
 *   r4 = final kernel address
 *   r5 = start of this image
 *   r2 = end of malloc space (and therefore this image)
 * We basically want:
 *   r4 >= r2 -> OK
 *   r4 + image length <= r5 -> OK
 */
	cmphs   r4, r2
	bt      wont_overwrite
	mov     r3, sp
	sub     r3, r5		// > compressed kernel size
	mov     r10, r4
	mov     r11, r4
	mov     r6, r2
	lsli    r3, 2
	add     r10, r3		// allow for 4x expansion
	cmplt   r10, r5
	bt      wont_overwrite
#ifdef CONFIG_CPU_CSKYV1
	mov     r3, r1
	mov     r4, r2
	mov     r5, r8
	jbsr    decompress_kernel
#else
	mov     r0, r2                  // decompress after malloc space
	mov     r3, r8
	jbsr    decompress_kernel
#endif
#ifndef CONFIG_CPU_CSKYV1
	mov     r2,r0
 	mov     r1, r6
#else
	mov     r1, sp
	lrw     r6, 0x10000
	add     r1, r6  // 64k max
	mov     r6, r1	
#endif

/* Get "Image" length, store in r2 */
#ifndef CONFIG_CPU_BIG_ENDIAN
	movi	r2, 0
	lrw	r7, _image_length_addr
	subi	r7, r7, 4
	ldb     r3, (r7, 0)
	or	r2, r3
	ldb     r3, (r7, 1)
	lsli	r3, 8
	or	r2, r3
	ldb     r3, (r7, 2)
	lsli	r3, 16
	or	r2, r3
	ldb     r3, (r7, 3)
	lsli	r3, 24
	or	r2, r3
#else
	movi	r2, 0
	lrw	r7, _image_length_addr
	subi	r7, r7, 4
	ldb     r3, (r7, 3)
	or	r2, r3
	ldb     r3, (r7, 2)
	lsli	r3, 8
	or	r2, r3
	ldb     r3, (r7, 1)
	lsli	r3, 16
	or	r2, r3
	ldb     r3, (r7, 0)
	lsli	r3, 24
	or	r2, r3		/* r2 = decompressed kernel length */
#endif
	mov     r4,r11
	lrw     r12, 255
	add     r2, r12	// alignment + stack
	lrw     r12, 0xFFFFFFFF
	lrw     r10,127
	subu    r12,r10
	and     r2,r12


/*
 * r7     = reloc_start's new start address
 */
	add     r1, r2		// end of decompressed kernel
	mov     r7, r1
	lrw     r2, reloc_start
	lrw     r3, reloc_end
1:	ldw     r10,(r2,0)	// copy relocation code
	stw     r10,(r1,0)
	addi    r2,4
	addi    r1,4
	cmplt   r2, r3
	bt      1b
	mov     sp, r1
	lrw     r10,128
	add     sp, r10		// relocate the stack
	jmp     r7
wont_overwrite:	
#ifndef CONFIG_CPU_CSKYV1
	mov     r4, r2
	lrw     r2, zreladdr
	mov     r3, r1
	mov     r5, r8
	jbsr    decompress_kernel
	mov     r0,r2	
#else
	mov     r0, r4
	mov     r3, r8
	jbsr    decompress_kernel
#endif
	jmpi    call_kernel
	.align  2
	.type   LC0, object
LC0:	.long	LC0			// r1
	.long	_sbss			// r2
	.long	_ebss			// r3
	.long	zreladdr		// r4
	.long	_start			// r5
	.long	_got_start		// r6
	.long	_got_end		// r7
	.long	user_stack+4096		// r14
LC1:	.long	reloc_end - reloc_start
	.size	LC0, . - LC0

/*
 * All code following this line is relocatable.  It is relocated by
 * the above code to the end of the decompressed kernel image and
 * executed there.  During this time, we have no stacks.
 *
 * r0     = decompressed kernel length
 * r1-r3  = unused
 * r4     = kernel execution address
 * r5     = decompressed kernel start
 * r6     = processor ID
 * r7     = architecture ID
 * r8     = atags pointer
 * r9-r12,r14 = corrupted
 */
               .align  5
reloc_start:
	mov     r3, r7
	bgeni   r10, 7
	sub     r3, r10         // do not copy the stack
	mov     r1, r4
1:
	ldw     r10, (r6,0)    // relocate kernel
	stw     r10, (r1,0)
	addi    r6,4
	addi    r1,4
	cmplt   r6, r3
	bt      1b
	mov     sp, r1
	bgeni   r10, 7
	add     sp, r10         // relocate the stack

	/*
	 *      Assember start up done, start code proper.
	 */
call_kernel:
	//Invalid instruction cache
	movi    r10, 0x11
	mtcr    r10, cr17
	//Invalid Data cache
	movi    r10, 0x12
	mtcr    r10, cr17
	mfcr    r7, cr18
	bclri   r7, 0                   // bit 0~1, mmu/mgu enable
	bclri   r7, 1                   //
	bclri   r7, 2                   // instruction cache
	bclri   r7, 3                   // data cache
	bclri   r7, 6                   // jumping predict
	mtcr    r7, cr18
	movi    r0, 0
	mfcr    r2, ss2			// restore atags pointer
	mfcr    r3, ss3
	jmp     r4			// call kernel

__default_exception_handler:
	bkpt
	br      __default_exception_handler
	rte

reloc_end:
	.align
	.section ".stack", "w"
user_stack:	.space	4096
1:
	br      1b				        /* Should never get here */

