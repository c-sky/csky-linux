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
 * Generic memory range, used by several tags.
 *
 *   addr is always physical.
 *   size is measured in bytes.
 *   next is for use by the OS, e.g. for grouping regions into
 *        linked lists.
 */
struct tag_mem_range {
	u32	addr;
	u32	size;
	long	type;	/* type of memory segment */
};

/* The list ends with an ATAG_NONE node. */
#define ATAG_NONE	0x00000000

struct tag_header {
	u32 size;
	u32 tag;
};

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE	0x54410001

struct tag_core {
	u32 flags;
	u32 pagesize;
	u32 rootdev;
};

/* it is allowed to have multiple ATAG_MEM nodes */
#define ATAG_MEM	0x54410002
/* ATAG_MEM uses tag_mem_range */

/* command line: \0 terminated string */
#define ATAG_CMDLINE	0x54410003

struct tag_cmdline {
	char	cmdline[1];	/* this is the minimum size */
};

/* Ramdisk image (may be compressed) */
#define ATAG_RDIMG	0x54410004
/* ATAG_RDIMG uses tag_mem_range */

/* Information about various clocks present in the system */
#define ATAG_CLOCK	0x54410005

struct tag_clock {
	u32	clock_id;	/* Which clock are we talking about? */
	u32	clock_flags;	/* Special features */
	u64	clock_hz;	/* Clock speed in Hz */
};

/* The clock types we know about */
#define CLOCK_BOOTCPU	0

/* Memory reserved for the system (e.g. the bootloader) */
#define ATAG_RSVD_MEM	0x54410006
/* ATAG_RSVD_MEM uses tag_mem_range */

struct tag {
	struct tag_header hdr;
	union {
		struct tag_core core;
		struct tag_mem_range mem_range;
		struct tag_cmdline cmdline;
		struct tag_clock clock;
	} u;
};

struct tagtable {
	u32	tag;
	int	(*parse)(struct tag *);
};

#define __tag __used __attribute__((__section__(".taglist.init")))
#define __tagtable(tag, fn)						\
	static struct tagtable __tagtable_##fn __tag = { tag, fn }

#define tag_member_present(tag,member)					\
	((unsigned long)(&((struct tag *)0L)->member + 1)		\
	 <= (tag)->hdr.size * 4)

#define tag_next(t)	((struct tag *)((u32 *)(t) + (t)->hdr.size))
#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define for_each_tag(t,base)						\
	for (t = base; t->hdr.size; t = tag_next(t))

extern struct tag *bootloader_tags;

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
