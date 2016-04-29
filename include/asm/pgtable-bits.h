#ifndef __ASM_CSKY_PGTABLE_BITS_H
#define __ASM_CSKY_PGTABLE_BITS_H

/*
 * Note that we shift the lower 32bits of each EntryLo[01] entry
 * 6 bits to the left. That way we can convert the PFN into the
 * physical address by a single 'and' operation and gain 6 additional
 * bits for storing information which isn't present in a normal
 * CSKY page table.
 *
 * Similar to the Alpha port, we need to keep track of the ref
 * and mod bits in software.  We have a software "yeah you can read
 * from this page" bit, and a hardware one which actually lets the
 * process read from the page.  On the same token we have a software
 * writable bit and the real hardware one which actually lets the
 * process write to the page, this keeps a mod bit via the hardware
 * dirty bit.
 *
 * Certain revisions of the ck640 have a bug where if a
 * certain sequence occurs in the last 3 instructions of an executable
 * page, and the following page is not mapped, the cpu can do
 * unpredictable things.  The code (when it is written) to deal with
 * this problem will be in the update_mmu_cache() code for the r4k.
 */

#ifdef CONFIG_CPU_CSKYV1
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
#else
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
#endif

#define __READABLE	(_PAGE_READ | _PAGE_SILENT_READ | _PAGE_ACCESSED)
#define __WRITEABLE	(_PAGE_WRITE | _PAGE_SILENT_WRITE | _PAGE_MODIFIED)

#define _PAGE_CHG_MASK  (PAGE_MASK | _PAGE_ACCESSED | \
                             _PAGE_MODIFIED | _CACHE_MASK)

#ifdef CONFIG_CSKY_USER_SEGMENT_CACHE
#define PAGE_CACHABLE_DEFAULT	_CACHE_CACHED
#else
#define PAGE_CACHABLE_DEFAULT   _CACHE_UNCACHED
#endif

#define CONF_CM_DEFAULT		(PAGE_CACHABLE_DEFAULT >> 3)

#endif /* __ASM_CSKY_PGTABLE_BITS_H */
