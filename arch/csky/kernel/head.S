#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>

__INIT
ENTRY(_start)
	/* set super user mode */
	lrw	a3, 0x80000000
	mtcr    a3, psr
	psrset  ee

	/* set stack point */
	lrw     a3, init_thread_union + THREAD_SIZE
	mov	sp, a3

	jmpi	csky_start
END(_start)

