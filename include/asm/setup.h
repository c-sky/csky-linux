#ifndef __ASM_CSKY_SETUP_H
#define __ASM_CSKY_SETUP_H

#include <linux/types.h>
#define BOOT_MEM_MAP_MAX        8
#define BOOT_MEM_RAM            1
#define BOOT_MEM_ROM_DATA       2
#define BOOT_MEM_RESERVED       3

#ifdef __KERNEL__

/* Magic number indicating that a tag table is present */
#define ATAG_MAGIC	0xa2a25441

#include <uapi/asm-generic/setup.h>

#ifndef __ASSEMBLY__

/*
 * A memory map that's built upon what was determined
 * or specified on the command line.
 */
struct boot_mem_map {
        int nr_map;
        struct boot_mem_map_entry {
                phys_addr_t addr;	/* start of memory segment */
                phys_addr_t size;	/* size of memory segment */
                long type;	/* type of memory segment */
        } map[BOOT_MEM_MAP_MAX];
};

extern struct boot_mem_map boot_mem_map;

extern void add_memory_region(phys_addr_t start, phys_addr_t size, long type);

#endif /* !__ASSEMBLY__ */
#endif  /* __KERNEL__ */
#endif /* __ASM_CSKY_SETUP_H */
