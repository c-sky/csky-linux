#ifndef __ASM_CSKY_PGTABLE_BITS_H
#define __ASM_CSKY_PGTABLE_BITS_H

#define _PAGE_PRESENT		(1<<0) /* implemented in software */
#define _PAGE_READ		(1<<1) /* implemented in software */
#define _PAGE_WRITE		(1<<2) /* implemented in software */
#define _PAGE_ACCESSED		(1<<3) /* implemented in software */
#define _PAGE_MODIFIED		(1<<4) /* implemented in software */

#define _PAGE_GLOBAL		(1<<6)
#define _PAGE_VALID		(1<<7)
#define _PAGE_DIRTY		(1<<8)
#define _PAGE_CACHE		(3<<9)
#define _PAGE_UNCACHE		(2<<9)

#define _CACHE_MASK		(7<<9)

#define _CACHE_CACHED		(_PAGE_VALID | _PAGE_CACHE)
#define _CACHE_UNCACHED		(_PAGE_VALID | _PAGE_UNCACHE)

#define HAVE_ARCH_UNMAPPED_AREA

#endif /* __ASM_CSKY_PGTABLE_BITS_H */
