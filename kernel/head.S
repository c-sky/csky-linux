#include <asm/page.h>

_start:
	/* setup super user */
	lrw	a5, 0x80000000
	mtcr    a5, psr

	/* setup sp */
	lrw     a5, init_thread_union + THREAD_SIZE
	mov	sp, a5

	jsri	pre_start
	jmpi	start_kernel

.export _start

