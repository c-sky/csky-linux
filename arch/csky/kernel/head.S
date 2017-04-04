#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/page.h>
#include <hal/regdef.h>

ENTRY(_start)
	/* set super user mode */
	lrw	a3, DEFAULT_PSR_VALUE
	mtcr    a3, psr
	psrset  ee

	/* set stack point */
	lrw     a3, init_thread_union + THREAD_SIZE
	mov	sp, a3

	jmpi	csky_start
END(_start)

