/*
 * linux/arch/csky/kernel/setup.c
 * This file handles the architecture-dependent parts of system setup
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2006  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2006  Li Chunqiang (chunqiang_li@c-sky.com)
 * Copyright (C) 2009  Hu junshan (junshan_hu@c-sky.com)
 *
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/console.h>
#include <linux/genhd.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/bootmem.h>
#include <linux/highmem.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/initrd.h>
#include <linux/root_dev.h>
#include <linux/rtc.h>
#include <linux/screen_info.h>

#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/cpu.h>
#include <asm/machdep.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <asm/pgtable.h>
#endif

extern void cpu_probe(void);
extern void cpu_report(void);
extern void paging_init(void);
extern void setup_memory(void);

extern char __cpu_name[NR_CPUS][32];
struct boot_mem_map boot_mem_map;

static char __initdata command_line[COMMAND_LINE_SIZE];
static char default_command_line[COMMAND_LINE_SIZE] __initdata = CONFIG_CMDLINE;

void (*mach_time_init) (void) __initdata = NULL;
void (*mach_time_suspend) (void) = NULL;
void (*mach_time_resume) (void) = NULL;
void (*mach_tick)( void ) = NULL;
int (*mach_keyb_init) (void);
unsigned long (*mach_gettimeoffset) (void) = NULL;
int  (*mach_set_clock_mmss) (unsigned long) = NULL;
int (*mach_hwclk) (int, struct rtc_time*) = NULL;
EXPORT_SYMBOL(mach_hwclk);
unsigned int (*mach_get_ss)(void);
int (*mach_get_rtc_pll)(struct rtc_pll_info *);
int (*mach_set_rtc_pll)(struct rtc_pll_info *);
EXPORT_SYMBOL(mach_get_ss);
EXPORT_SYMBOL(mach_get_rtc_pll);
EXPORT_SYMBOL(mach_set_rtc_pll);
void (*mach_trap_init) (void) = NULL;
void (*mach_init_IRQ) (void) __initdata = NULL;
unsigned int (*mach_get_auto_irqno) (void) = NULL;
void (*mach_init_FIQ) (void) __initdata = NULL;
unsigned int (*mach_get_auto_fiqno) (void) = NULL;
void (*mach_get_model) (char *model);
void (*mach_get_hardware_list) (struct seq_file *m);
void (*mach_reset)( void ) = NULL;
void (*mach_halt)( void ) = NULL;
void (*mach_power_off)( void ) = NULL;
#ifdef CONFIG_HEARTBEAT
void (*mach_heartbeat) (int);
EXPORT_SYMBOL(mach_heartbeat);
#endif

unsigned int system_rev;
EXPORT_SYMBOL(system_rev);

#if defined(CONFIG_VGA_CONSOLE) || defined(CONFIG_DUMMY_CONSOLE)
struct screen_info screen_info = {
 .orig_video_lines      = 15,
 .orig_video_cols       = 40,
 .orig_video_mode       = 0,
 .orig_video_ega_bx     = 0,
 .orig_video_isVGA      = 1,
 .orig_video_points     = 8
};
#endif

static struct resource code_resource = { .name = "Kernel code", };
static struct resource data_resource = { .name = "Kernel data", };

void __init add_memory_region(phys_addr_t start, phys_addr_t size,
			      long type)
{
	int x = boot_mem_map.nr_map;

	struct boot_mem_map_entry *prev = boot_mem_map.map + x - 1;

	/* Sanity check */
	if (start + size < start) {
		pr_warning("Trying to add an invalid memory region, skipped\n");
		return;
	}

	/*
	 * Try to merge with previous entry if any.  This is far less than
	 * perfect but is sufficient for most real world cases.
	 */
	if (x && prev->addr + prev->size == start && prev->type == type) {
		prev->size += size;
		return;
	}

	if (x == BOOT_MEM_MAP_MAX) {
		printk("Ooops! Too many entries in the memory map!\n");
		return;
	}

	boot_mem_map.map[x].addr = start;
	boot_mem_map.map[x].size = size;
	boot_mem_map.map[x].type = type;
	boot_mem_map.nr_map++;
}

static void __init print_memory_map(void)
{
	int i;

	for (i = 0; i < boot_mem_map.nr_map; i++) {
		printk(" memory: %08Lx @ %08Lx ",
			  (unsigned long long) boot_mem_map.map[i].size,
		          (unsigned long long) boot_mem_map.map[i].addr);

		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
			printk("(usable)\n");
			break;
		case BOOT_MEM_ROM_DATA:
			printk("(ROM data)\n");
			break;
		case BOOT_MEM_RESERVED:
			printk("(reserved)\n");
			break;
		default:
			printk("type %lu\n", boot_mem_map.map[i].type);
			break;
		}
	}
}

/*
 * Pick out the memory size.  We look for mem=size@start,
 * where start and size are "size[KkMm]"
 */
static int usermem __initdata;
static int __init early_mem(char *p)
{
	unsigned long size, start;
	/*
	 * If the user specifies memory size, we
	 * blow away any automatically generated
	 * size.
	 */
	if (usermem == 0) {
		usermem = 1;
		boot_mem_map.nr_map = 0;
	}

	start = PHY_OFFSET;
	size  = memparse(p, &p);
	if (*p == '@')
		start = memparse(p + 1, &p);

	add_memory_region(start, size, BOOT_MEM_RAM);
	return 0;
}
early_param("mem", early_mem);

static int __init early_highmem(char *p)
{
	unsigned long size, start;
	/*
	 * If the user specifies memory size, we
	 * blow away any automatically generated
	 * size.
	 */
	if (usermem == 0) {
		usermem = 1;
		boot_mem_map.nr_map = 0;
	}

	start = HIGHMEM_START;
	size  = memparse(p, &p);
	if (*p == '@')
		start = memparse(p + 1, &p);

	add_memory_region(start, size, BOOT_MEM_RAM);
	return 0;
}
early_param("highmem", early_highmem);

static int __init parse_tag_core(struct tag *tag)
{
	if (tag->hdr.size > 2) {
		if ((tag->u.core.flags & 1) == 0)
			root_mountflags &= ~MS_RDONLY;
		ROOT_DEV = old_decode_dev(tag->u.core.rootdev);
	}
	return 0;
}
__tagtable(ATAG_CORE, parse_tag_core);

static int __init parse_tag_mem(struct tag *tag)
{
	/*
	 * Ignore zero-sized entries. If we're running standalone, the
	 * SDRAM code may emit such entries if something goes
	 * wrong...
	 */
	if (tag->u.mem_range.size == 0)
		return 0;

	add_memory_region(tag->u.mem_range.addr, tag->u.mem_range.size,
		tag->u.mem_range.type);
	return 0;
}
__tagtable(ATAG_MEM, parse_tag_mem);

static int __init parse_tag_rdimg(struct tag *tag)
{
#ifdef CONFIG_BLK_DEV_INITRD
	struct tag_mem_range *mem = &tag->u.mem_range;

	if (initrd_start) {
		printk(KERN_WARNING
		       "Warning: Only the first initrd image will be used\n");
		return 0;
	}

	initrd_start = (unsigned long)__va(mem->addr);
	initrd_end = initrd_start + mem->size;
#else
	printk(KERN_WARNING "RAM disk image present, but "
	       "no initrd support in kernel, ignoring\n");
#endif

	return 0;
}
__tagtable(ATAG_RDIMG, parse_tag_rdimg);

static int __init parse_tag_cmdline(struct tag *tag)
{
	strlcpy(default_command_line, tag->u.cmdline.cmdline, COMMAND_LINE_SIZE);
	return 0;
}
__tagtable(ATAG_CMDLINE, parse_tag_cmdline);

static int __init parse_tag_clock(struct tag *tag)
{
	/*
	 * We'll figure out the clocks by peeking at the system
	 * manager regs directly.
	 */
	return 0;
}
__tagtable(ATAG_CLOCK, parse_tag_clock);

/*
 * Scan the tag table for this tag, and call its parse function. The
 * tag table is built by the linker from all the __tagtable
 * declarations.
 */
static int __init parse_tag(struct tag *tag)
{
	extern struct tagtable __tagtable_begin, __tagtable_end;
	struct tagtable *t;

	for (t = &__tagtable_begin; t < &__tagtable_end; t++)
		if (tag->hdr.tag == t->tag) {
			t->parse(tag);
			break;
		}

	return t < &__tagtable_end;
}

/*
 * Parse all tags in the list we got from the boot loader
 */
static void __init parse_tags(struct tag *t)
{
	for (; t->hdr.tag != ATAG_NONE; t = tag_next(t))
		if (!parse_tag(t))
			printk(KERN_WARNING
			       "Ignoring unrecognised tag 0x%08x\n",
			       t->hdr.tag);
}

#ifdef CONFIG_BLK_DEV_INITRD
/* it returns the next free pfn after initrd */
static unsigned long __init init_initrd(void)
{
	unsigned long end;

	/*
	 * Board specific code or command line parser should have
	 * already set up initrd_start and initrd_end. In these cases
	 * perfom sanity checks and use them if all looks good.
	 */
	if (!initrd_start || initrd_end <= initrd_start) {
		goto disable;
	}

	if (initrd_start & ~PAGE_MASK) {
		pr_err("initrd start must be page aligned\n");
		goto disable;
	}
	if (initrd_start < PAGE_OFFSET) {
		pr_err("initrd start < PAGE_OFFSET\n");
		goto disable;
	}

	/*
	 * Sanitize initrd addresses. For example firmware
	 * can't guess if they need to pass them through
	 * 64-bits values if the kernel has been built in pure
	 * 32-bit. We need also to switch from KSEG0 to XKPHYS
	 * addresses now, so the code can now safely use __pa().
	 */
	end = __pa(initrd_end);

	ROOT_DEV = Root_RAM0;
	return PFN_UP(end);
disable:
	initrd_start = 0;
	initrd_end = 0;
	return 0;
}

static void __init finalize_initrd(void)
{
	unsigned long size = initrd_end - initrd_start;


	if (size == 0) {
		printk(KERN_INFO "Initrd not found or empty");
		goto disable;
	}

	if (__pa(initrd_end) > PFN_PHYS(max_low_pfn)) {
		printk(KERN_ERR "Initrd extends beyond end of memory");
		goto disable;
	}

	reserve_bootmem(__pa(initrd_start), size, BOOTMEM_DEFAULT);
	initrd_below_start_ok = 1;

	if(initrd_start > (unsigned long)_end)
		free_bootmem(__pa_symbol(&_end), initrd_start - (unsigned long)_end);

	pr_info("Initial ramdisk at: 0x%lx (%lu bytes)\n",
		initrd_start, size);
	return;
disable:
	printk(KERN_CONT " - disabling initrd\n");
	initrd_start = 0;
	initrd_end = 0;
}
#else  /* !CONFIG_BLK_DEV_INITRD */
static unsigned long __init init_initrd(void)
{
	return 0;
}

#define finalize_initrd()       do {} while (0)
#endif
/*
 * Initialize the bootmem allocator. It also setup initrd related data
 * if needed.
 */
static void __init bootmem_init(void)
{
	unsigned long reserved_end;
	unsigned long mapstart = ~0UL;
	unsigned long bootmap_size;
	int i;

	/*
	 * Init any data related to initrd. It's a nop if INITRD is
	 * not selected. Once that done we can determine the low bound
	 * of usable memory.
	 */
	reserved_end = max(init_initrd(),
	                   (unsigned long) PFN_UP(__pa_symbol(&_end)));

	/*
	 * max_low_pfn is not a number of pages. The number of pages
	 * of the system is given by 'max_low_pfn - min_low_pfn'.
	 */
	min_low_pfn = ~0UL;
	max_low_pfn = 0;

	/*
	 * Find the highest page frame number we have available.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end;

		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
		        continue;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end = PFN_DOWN(boot_mem_map.map[i].addr
		                + boot_mem_map.map[i].size);

		if (end > max_low_pfn)
		        max_low_pfn = end;
		if (start < min_low_pfn)
		        min_low_pfn = start;
		if (end <= reserved_end)
		        continue;
		if (start >= mapstart)
		        continue;
		mapstart = max(reserved_end, start);
	}

	if (min_low_pfn >= max_low_pfn)
	        panic("Incorrect memory mapping !!!");
	if (min_low_pfn > ARCH_PFN_OFFSET) {
		pr_info("Wasting %lu bytes for tracking %lu unused pages\n",
	                (min_low_pfn - ARCH_PFN_OFFSET) * sizeof(struct page),
	                min_low_pfn - ARCH_PFN_OFFSET);
	} else if (min_low_pfn < ARCH_PFN_OFFSET) {
		pr_info("%lu free pages won't be used\n",
	                ARCH_PFN_OFFSET - min_low_pfn);
	}
	min_low_pfn = ARCH_PFN_OFFSET;

	/*
	 * Determine low and high memory ranges
	 */
	max_pfn = max_low_pfn;
	if (max_low_pfn > PFN_DOWN(HIGHMEM_START)) {
#ifdef CONFIG_HIGHMEM
		highstart_pfn = PFN_DOWN(HIGHMEM_START);
		highend_pfn = max_low_pfn;
#endif
		max_low_pfn = PFN_DOWN(HIGHMEM_START);
	}

	/*
	 * Initialize the boot-time allocator with low memory only.
	 */
	bootmap_size = init_bootmem_node(NODE_DATA(0), mapstart,
	                                 min_low_pfn, max_low_pfn);


	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end = PFN_DOWN(boot_mem_map.map[i].addr
		                + boot_mem_map.map[i].size);

		if (start <= min_low_pfn)
		        start = min_low_pfn;
		if (start >= end)
		        continue;
#ifndef CONFIG_HIGHMEM
		if (end > max_low_pfn)
		        end = max_low_pfn;

		/*
		 * ... finally, is the area going away?
		 */
		if (end <= start)
		        continue;
#endif

		memblock_add_node(PFN_PHYS(start), PFN_PHYS(end - start), 0);
	}

	/*
	 * Register fully available low RAM pages with the bootmem allocator.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		unsigned long start, end, size;

		/*
		 * Reserve usable memory.
		 */
		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
		        continue;

		start = PFN_UP(boot_mem_map.map[i].addr);
		end   = PFN_DOWN(boot_mem_map.map[i].addr
		                    + boot_mem_map.map[i].size);
		/*
		 * We are rounding up the start address of usable memory
		 * and at the end of the usable range downwards.
		 */
		if (start >= max_low_pfn)
		        continue;
		if (start < reserved_end)
		        start = reserved_end;
		if (end > max_low_pfn)
		        end = max_low_pfn;


		/*
		 * ... finally, is the area going away?
		 */
		if (end <= start)
		        continue;
		size = end - start;

		/* Register lowmem ranges */
		free_bootmem(PFN_PHYS(start), size << PAGE_SHIFT);
		memory_present(0, start, end);
	}

	/*
	 * Reserve the bootmap memory.
	 */
	reserve_bootmem(PFN_PHYS(mapstart), bootmap_size, BOOTMEM_DEFAULT);

	/*
	 * Reserve initrd memory if needed.
	 */
	finalize_initrd();
}

unsigned long os_config_fcr;

/* enable and init FPU */
static inline void init_fpu(void)
{
	unsigned long flg;
	unsigned long cpwr, fcr;

	cpwr = 0xf0000007; // set for reg CPWR(cp15): ie, ic, ec, rp, wp, en = 1
	os_config_fcr = (IDE_STAT | IXE_STAT | UFE_STAT | OFE_STAT | DZE_STAT | IOE_STAT);
	fcr = os_config_fcr;
	local_save_flags(flg);
#if defined(CONFIG_CPU_CSKYV1)
	__asm__ __volatile__("cpseti  1 \n\t"
	                     "mtcr    %0, cr15 \n\t"
	                     "cpwcr   %1, cpcr1 \n\t"
	                     ::"r"(cpwr), "b"(fcr)
	                     );
#else
	__asm__ __volatile__("mtcr    %0, cr<1, 2> \n\t"
			     ::"r"(fcr)
			    );
#endif

	local_irq_restore(flg);
}

static void __init resource_init(void)
{
	int i;

	code_resource.start = __pa_symbol(&_text);
	code_resource.end = __pa_symbol(&_etext) - 1;
	data_resource.start = __pa_symbol(&_etext);
	data_resource.end = __pa_symbol(&_edata) - 1;

	/*
	 * Request address space for all standard RAM.
	 */
	for (i = 0; i < boot_mem_map.nr_map; i++) {
		struct resource *res;
		unsigned long start, end;

		start = boot_mem_map.map[i].addr;
		end = boot_mem_map.map[i].addr + boot_mem_map.map[i].size - 1;
		if (start >= HIGHMEM_START)
			continue;
		if (end >= HIGHMEM_START)
			end = HIGHMEM_START - 1;

		res = alloc_bootmem(sizeof(struct resource));
		switch (boot_mem_map.map[i].type) {
		case BOOT_MEM_RAM:
		case BOOT_MEM_ROM_DATA:
			res->name = "System RAM";
			break;
		case BOOT_MEM_RESERVED:
		default:
			res->name = "reserved";
		}

		res->start = start;
		res->end = end;

		res->flags = IORESOURCE_MEM | IORESOURCE_BUSY;
		request_resource(&iomem_resource, res);

		/*
		 *  We don't know which RAM region contains kernel data,
		 *  so we try it repeatedly and let the resource manager
		 *  test it.
		 */
		request_resource(res, &code_resource);
		request_resource(res, &data_resource);
	}
}

void __init setup_arch(char **cmdline_p)
{
#ifdef CONFIG_MMU
	printk("Linux C-SKY port done by C-SKY Microsystems co.,ltd.  www.c-sky.com\n");
#else
	printk("uClinux C-SKY port done by C-SKY Microsystems co.,ltd.  www.c-sky.com\n");
#endif

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0;

	cpu_probe();
	if(bootloader_tags)
		parse_tags(bootloader_tags);
	config_BSP();
	cpu_report();

#ifdef CONFIG_BLK_DEV_BLKMEM
	ROOT_DEV = BLKMEM_MAJOR;
	ROOT_DEV <<= MINORBITS;
#endif

	pr_info("Determined physical RAM map:\n");
	print_memory_map();

	/* Keep a copy of command line */
	strlcpy(command_line, default_command_line, sizeof(command_line));
	strlcpy(boot_command_line, command_line, COMMAND_LINE_SIZE);

	*cmdline_p = command_line;

	parse_early_param();

	if (usermem) {
		pr_info("User-defined physical RAM map:\n");
		print_memory_map();
	}

#ifdef CONFIG_VT
#if defined(CONFIG_VGA_CONSOLE)
        conswitchp = &vga_con;
#elif defined(CONFIG_DUMMY_CONSOLE)
        conswitchp = &dummy_con;
#endif
#endif

	setup_memory();  // Revise memory region when nommu
#ifndef CONFIG_MMU
	pr_info("Real physical RAM map:\n");
	print_memory_map();
#endif

	bootmem_init();  // setup the bitmap for all the whole ram
	sparse_init();
	paging_init();
	resource_init();

#ifdef CONFIG_CPU_HAS_FPU
	init_fpu();
#endif
}

/*
 *	Get CPU information for use by the procfs.
 */

static int show_cpuinfo(struct seq_file *m, void *v)
{
    unsigned long n = (unsigned long) v - 1;
    char *fpu;
    u_long clockfreq;
    struct cpuinfo_csky * c=&cpu_data[n];
    fpu = "none";

    seq_printf(m, "Processor\t: %ld\n", n);
    seq_printf(m, "CPU\t\t: %s(0x%8x)\n", __cpu_name[n], c->processor_id[0]);

	if(c->fpu_id) {
        fpu = (c->fpu_id == CPUID_FPU_V1 ? "V1(FPU)" : "V2(VFP)");
    }

    /*
     * For the first processor also print the system type and version
     */
    if (n == 0) seq_printf(m, "Revision\t: %04x\n", system_rev);

    /*
     * The fiducial operation declt + bf need 2 cycle. So calculate CPU clock
     *  need to multiply 2.
     */
    clockfreq = (loops_per_jiffy*HZ)*2;
    seq_printf(m, "FPU\t\t: %s\n"
		 "Clocking\t: %lu.%1luMHz\n"
		 "BogoMips\t: %lu.%02lu\n"
		 "Calibration\t: %lu loops\n",
		 fpu, clockfreq / 1000000, (clockfreq / 10000) % 100,
		 (loops_per_jiffy * HZ) / 500000, ((loops_per_jiffy * HZ) / 5000) % 100,
		 (loops_per_jiffy * HZ));
    seq_printf(m, "Dcache_size\t: %dKByte\n", ((c->cache_size >> 16) & 0xffff));
    seq_printf(m, "Icache_size\t: %dKByte\n", (c->cache_size & 0xffff));
    seq_printf(m, "Dsram_size\t: %dKByte\n", ((c->sram_size >> 16) & 0xffff));
    seq_printf(m, "Isram_size\t: %dKByte\n", (c->sram_size & 0xffff));

    return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	unsigned long i = *pos;

	return i < NR_CPUS ? (void *) (i + 1) : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

struct seq_operations cpuinfo_op = {
	start:	c_start,
	next:	c_next,
	stop:	c_stop,
	show:	show_cpuinfo,
};

