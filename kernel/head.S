#include <linux/init.h>
#include <linux/threads.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/entry.h>
#include <asm/regdef.h>

.export _ramvec
.export swapper_pg_dir
.export invalid_pte_table

.export _start
.export _stext

.data

_ramvec:
.long	0

.align 12
swapper_pg_dir:
.fill	4096
invalid_pte_table:
.fill	4096

.align 10
virtual_vec_base:
.fill	128, 4

__HEAD
_stext:
_start:
	movi    r7, 0x33
	mtcr    r7, cr17

	lrw     a0, init_thread_union
	lrw     r4, THREAD_SIZE 
	addu    r4, a0
	mov	sp, r4

	lrw	r4, 0x8f000100
	mtcr    r4, psr

	SET_CP_MMU

	movi    r7, 0x00000000 //VPN2 = 0x0, ASID = 0
	WR_MEH	r7
	bgeni   r4, 26
	WR_MCIR r4

	movi	r4, 0
	WR_MPR	r4              // page mask = 4k

	mfcr	r7, cr17
	bseti   r7, 16   // invalid BHT
	bseti   r7, 17   // invalid BTB
	mtcr    r7, cr17

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
 	subi    a0, 1
        cmpnei  a0, 0
	bt      1b                      /* Repeat for all bss */

	jmpi	start_kernel

