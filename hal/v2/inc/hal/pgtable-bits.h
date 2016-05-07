#ifndef __ASM_CSKY_PGTABLE_BITS_H
#define __ASM_CSKY_PGTABLE_BITS_H

#define _PAGE_ACCESSED              (1<<7)  /* implemented in software */
#define _PAGE_READ                  (1<<8)  /* implemented in software */
#define _PAGE_WRITE                 (1<<9)  /* implemented in software */
#define _PAGE_PRESENT               (1<<10)  /* implemented in software */
#define _PAGE_MODIFIED              (1<<11)  /* implemented in software */
#define _PAGE_FILE                  (1<<11)  /* set:pagecache unset:swap */
#define _PAGE_GLOBAL                (1<<0)
#define _PAGE_VALID                 (1<<1)
#define _PAGE_SILENT_READ           (1<<1)  /* synonym                 */
#define _PAGE_DIRTY                 (1<<2)  /* The CSKY dirty bit      */
#define _PAGE_SILENT_WRITE          (1<<2)
#define _CACHE_MASK                 (7<<3)

#define _CACHE_UNCACHED             (0x2)
#define _CACHE_CACHED               (0xa)

#define pte_to_pgoff(_pte) \
	(((_pte).pte_low & 0x3ff) | (((_pte).pte_low >> 12) << 10))
#define pgoff_to_pte(off) \
	((pte_t) {((off) & 0x3ff) | (((off) >> 10) << 12) | _PAGE_FILE})

#endif /* __ASM_CSKY_PGTABLE_BITS_H */
