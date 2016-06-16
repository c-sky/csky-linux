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
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>

#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/cpu.h>
#include <asm/machdep.h>
#include <asm/mmu_context.h>

#ifdef CONFIG_BLK_DEV_INITRD
#include <asm/pgtable.h>
#endif

extern void cpu_probe(void);
extern void cpu_report(void);
extern void paging_init(void);

struct boot_mem_map boot_mem_map;

void (*mach_init_IRQ) (void) __initdata = NULL;
unsigned int (*mach_get_auto_irqno) (void) = NULL;

static struct resource code_resource = { .name = "Kernel code", };
static struct resource data_resource = { .name = "Kernel data", };

void __init
add_memory_region(
	phys_addr_t start,
	phys_addr_t size,
	long type
	)
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

void __init early_init_dt_add_memory_arch(u64 base, u64 size)
{
	add_memory_region(base, size, BOOT_MEM_RAM);
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
	 * Initialize the boot-time allocator with low memory only.
	 */
	bootmap_size = init_bootmem_node(NODE_DATA(0), mapstart,
	                                 min_low_pfn, max_low_pfn);


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

	code_resource.start = __pa_symbol(&_stext);
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
	printk("Linux C-SKY port done by C-SKY Microsystems co.,ltd. www.c-sky.com\n");

	init_mm.start_code = (unsigned long) &_stext;
	init_mm.end_code = (unsigned long) &_etext;
	init_mm.end_data = (unsigned long) &_edata;
	init_mm.brk = (unsigned long) 0;

	cpu_probe();
	cpu_report();

#ifdef CONFIG_BLK_DEV_BLKMEM
	ROOT_DEV = BLKMEM_MAJOR;
	ROOT_DEV <<= MINORBITS;
#endif

	pr_info("Determined physical RAM map:\n");
	print_memory_map();
	*cmdline_p = boot_command_line;

	parse_early_param();

	pr_info("User-defined physical RAM map:\n");
	print_memory_map();

	/* setup bitmap for ram */
	bootmem_init();

	unflatten_and_copy_device_tree();

	sparse_init();
	paging_init();
	resource_init();

#ifdef CONFIG_CPU_HAS_FPU
	init_fpu();
#endif

#if defined(CONFIG_VT) && defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
}

static int __init customize_machine(void)
{
	of_platform_default_populate(NULL, NULL, NULL);
	return 0;
}
arch_initcall(customize_machine);

/*
 * Call from head.S before start_kernel, prepare vbr mmu bss
 */
extern unsigned int _sbss, _ebss, vec_base;
asmlinkage void pre_start(unsigned int magic, void *param)
{
	int vbr = (int) &vec_base;

	/* Setup vbr reg */
	__asm__ __volatile__(
			"mtcr %0, vbr\n"
			::"b"(vbr));

	/* Setup mmu as coprocessor */
	select_mmu_cp();

	/* Setup page mask to 4k */
	write_mmu_pagemask(0);

	/* Clean up bss section */
	memset((void *)&_sbss, 0,
		(unsigned int)&_ebss - (unsigned int)&_sbss);

	if (magic == 0x20150401) {
		early_init_dt_scan(param);
	}

#ifdef CONFIG_CMDLINE
	strlcpy(boot_command_line,
		CONFIG_CMDLINE_STR, COMMAND_LINE_SIZE);
#endif

	return;
}

