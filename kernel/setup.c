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

extern void paging_init(void);

void __init early_init_dt_add_memory_arch(u64 base, u64 size)
{
//	memblock_add(base, size);
	memblock_add_node(base, size, 0);
	max_low_pfn = PFN_DOWN(base + size);
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

	memblock_reserve(__pa(initrd_start), initrd_end - initrd_start);
	initrd_below_start_ok = 1;

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

	/*
	 * Init any data related to initrd. It's a nop if INITRD is
	 * not selected. Once that done we can determine the low bound
	 * of usable memory.
	 */
	reserved_end = max(init_initrd(),
	                   (unsigned long) PFN_UP(__pa_symbol(&_end)));


	min_low_pfn = ARCH_PFN_OFFSET;
	memblock_reserve(PHY_OFFSET, __pa(_end) - PHY_OFFSET);
	/*
	 * Reserve initrd memory if needed.
	 */
	finalize_initrd();
	early_init_fdt_reserve_self();
	memblock_dump_all();
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

void __init setup_arch(char **cmdline_p)
{
	*cmdline_p = boot_command_line;

	printk("www.c-sky.com\n");

	init_mm.start_code = (unsigned long) _text;
	init_mm.end_code = (unsigned long) _etext;
	init_mm.end_data = (unsigned long) _edata;
	init_mm.brk = (unsigned long) _end;

	parse_early_param();

	/* setup bitmap for ram */
	bootmem_init();

	unflatten_and_copy_device_tree();

	sparse_init();
	paging_init();

#ifdef CONFIG_CPU_HAS_FPU
	init_fpu();
#endif

#if defined(CONFIG_VT) && defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
}

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

	return;
}

