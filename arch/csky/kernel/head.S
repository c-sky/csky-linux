#include <linux/linkage.h>
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

