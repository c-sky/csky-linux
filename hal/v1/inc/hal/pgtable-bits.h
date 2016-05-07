#ifndef __ASM_CSKY_PGTABLE_BITS_H
#define __ASM_CSKY_PGTABLE_BITS_H

#define _PAGE_PRESENT               (1<<0)  /* implemented in software */
#define _PAGE_READ                  (1<<1)  /* implemented in software */
#define _PAGE_WRITE                 (1<<2)  /* implemented in software */
#define _PAGE_ACCESSED              (1<<3)  /* implemented in software */
#define _PAGE_MODIFIED              (1<<4)  /* implemented in software */
#define _PAGE_FILE                  (1<<4)  /* set:pagecache unset:swap */
#define _PAGE_GLOBAL                (1<<6)
#define _PAGE_VALID                 (1<<7)
#define _PAGE_SILENT_READ           (1<<7)  /* synonym                 */
#define _PAGE_DIRTY                 (1<<8)  /* The CSKY dirty bit      */
#define _PAGE_SILENT_WRITE          (1<<8)
#define _CACHE_MASK                 (7<<9)

#define _CACHE_UNCACHED             (0x12<<6)
#define _CACHE_CACHED               (0x1a<<6)

#define pte_to_pgoff(_pte) (pte.pte_low >> 4)
#define pgoff_to_pte(off) ((pte_t)((off << 4) + _PAGE_FILE))

#endif /* __ASM_CSKY_PGTABLE_BITS_H */
