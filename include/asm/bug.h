#ifndef __ASM_CSKY_BUG_H
#define __ASM_CSKY_BUG_H

#ifdef CONFIG_BUG

#define BUG() \
	do { \
		printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); \
		while(1); \
	} while (0)

#define HAVE_ARCH_BUG

#endif /* CONFIG_BUG */

#include <asm-generic/bug.h>

#endif /* __ASM_CSKY_BUG_H */
