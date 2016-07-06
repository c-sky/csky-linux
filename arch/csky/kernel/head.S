#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>

ENTRY(_start)
	/* set super user mode */
	lrw	a5, 0x80000000
	mtcr    a5, psr

	/* set stack point */
	lrw     a5, init_thread_union + THREAD_SIZE
	mov	sp, a5

	jsri	pre_start
	jmpi	start_kernel
END(_start)

__PAGE_ALIGNED_BSS
.balign PAGE_SIZE
ENTRY(empty_zero_page)
	.fill PAGE_SIZE
END(empty_zero_page)

