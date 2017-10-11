#include <linux/console.h>
#include <linux/memblock.h>
#include <linux/bootmem.h>
#include <linux/initrd.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/start_kernel.h>
#include <asm/sections.h>
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>

static void __init csky_memblock_init(void)
{
	unsigned long zone_size[MAX_NR_ZONES];

	memblock_reserve(__pa(_stext), _end - _stext);
#ifdef CONFIG_BLK_DEV_INITRD
	memblock_reserve(__pa(initrd_start), initrd_end - initrd_start);
#endif

	early_init_fdt_reserve_self();
	early_init_fdt_scan_reserved_mem();

	memblock_dump_all();

	/* free_area_init */
	memset(zone_size, 0, sizeof(zone_size));

	min_low_pfn = PFN_UP(memblock_start_of_DRAM());
	max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());

	zone_size[ZONE_NORMAL] = max_low_pfn - min_low_pfn;

	free_area_init_node(0, zone_size, min_low_pfn, NULL);
}

extern void cpu_dt_probe(void);
extern void init_fpu(void);
void __init setup_arch(char **cmdline_p)
{
	*cmdline_p = boot_command_line;

	printk("C-SKY: https://c-sky.github.io\n");

	init_mm.start_code = (unsigned long) _stext;
	init_mm.end_code = (unsigned long) _etext;
	init_mm.end_data = (unsigned long) _edata;
	init_mm.brk = (unsigned long) _end;

	parse_early_param();

	csky_memblock_init();

	unflatten_and_copy_device_tree();

	cpu_dt_probe();

	sparse_init();

	pgd_init((unsigned long)swapper_pg_dir);
	cache_op_all(DATA_CACHE|CACHE_CLR);

#if defined(CONFIG_VT) && defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
	init_fpu();
}

asmlinkage __visible void __init csky_start(
	unsigned int	magic,
	void *		param
	)
{
	/* Setup mmu as coprocessor */
	select_mmu_cp();

	/* Setup page mask to 4k */
	write_mmu_pagemask(0);

	/* Clean up bss section */
	memset(__bss_start, 0, __bss_stop - __bss_start);

#ifdef CONFIG_CSKY_BUILTIN_DTB
	printk("Use builtin dtb\n");
	early_init_dt_scan(__dtb_start);
#else
	if (magic == 0x20150401)
		early_init_dt_scan(param);
#endif
	start_kernel();

	while(1);
}

