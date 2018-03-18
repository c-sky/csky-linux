// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
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

phys_addr_t __init_memblock memblock_end_of_REG0(void)
{
	return (memblock.memory.regions[0].base + memblock.memory.regions[0].size);
}

phys_addr_t __init_memblock memblock_start_of_REG1(void)
{
	return memblock.memory.regions[1].base;
}

size_t __init_memblock memblock_size_of_REG1(void)
{
	return memblock.memory.regions[1].size;
}

static void __init csky_memblock_init(void)
{
	unsigned long zone_size[MAX_NR_ZONES];
	unsigned long zhole_size[MAX_NR_ZONES];
	signed long size;

	memblock_reserve(__pa(_stext), _end - _stext);
#ifdef CONFIG_BLK_DEV_INITRD
	memblock_reserve(__pa(initrd_start), initrd_end - initrd_start);
#endif

	early_init_fdt_reserve_self();
	early_init_fdt_scan_reserved_mem();

	memblock_dump_all();

	memset(zone_size, 0, sizeof(zone_size));
	memset(zhole_size, 0, sizeof(zhole_size));

	min_low_pfn = PFN_UP(memblock_start_of_DRAM());
	max_low_pfn = PFN_UP(memblock_end_of_REG0());
	max_pfn = PFN_DOWN(memblock_end_of_DRAM());

	size = max_pfn - min_low_pfn;

	if (memblock.memory.cnt > 1) {
		zone_size[ZONE_NORMAL]  = PFN_DOWN(memblock_start_of_REG1()) - min_low_pfn;
		zhole_size[ZONE_NORMAL] = PFN_DOWN(memblock_start_of_REG1()) - max_low_pfn;
	} else {
		if (size <= PFN_DOWN(LOWMEM_LIMIT - CONFIG_RAM_BASE))
			zone_size[ZONE_NORMAL] = max_pfn - min_low_pfn;
		else {
			zone_size[ZONE_NORMAL] = PFN_DOWN(LOWMEM_LIMIT - CONFIG_RAM_BASE);
			max_low_pfn = min_low_pfn + zone_size[ZONE_NORMAL];
		}
	}

#ifdef CONFIG_HIGHMEM
	size = 0;
	if(memblock.memory.cnt > 1) {
		size = PFN_DOWN(memblock_size_of_REG1());
		highstart_pfn = PFN_DOWN(memblock_start_of_REG1());
	} else {
		size = max_pfn - min_low_pfn - PFN_DOWN(LOWMEM_LIMIT - CONFIG_RAM_BASE);
		highstart_pfn =  min_low_pfn + PFN_DOWN(LOWMEM_LIMIT - CONFIG_RAM_BASE);
	}

	if (size > 0)
		zone_size[ZONE_HIGHMEM] = size;

	highend_pfn = max_pfn;
#endif
	memblock_set_current_limit(PFN_PHYS(max_low_pfn));

	free_area_init_node(0, zone_size, min_low_pfn, zhole_size);
}

extern void cpu_dt_probe(void);
void __init setup_arch(char **cmdline_p)
{
	*cmdline_p = boot_command_line;

	console_verbose();

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

#ifdef CONFIG_HIGHMEM
	kmap_init();
#endif
	cache_wbinv_all();

#if defined(CONFIG_VT) && defined(CONFIG_DUMMY_CONSOLE)
	conswitchp = &dummy_con;
#endif
}

extern void pre_trap_init(void);
asmlinkage __visible void __init csky_start(
	unsigned int	unused,
	void *		param
	)
{
	/* Clean up bss section */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	pre_trap_init();

	/* Setup mmu as coprocessor */
	select_mmu_cp();

	/*
	 * Setup page-table and enable TLB-hardrefill
	 */
	flush_tlb_all();
	pgd_init((unsigned long *)swapper_pg_dir);
	TLBMISS_HANDLER_SETUP_PGD(swapper_pg_dir);

	asid_cache(smp_processor_id()) = ASID_FIRST_VERSION;

	/* Setup page mask to 4k */
	write_mmu_pagemask(0);

#ifdef CONFIG_CSKY_BUILTIN_DTB
	printk("Use builtin dtb\n");
	early_init_dt_scan(__dtb_start);
#else
	early_init_dt_scan(param);
#endif
	printk("Phys. mem: %ldMB\n", (unsigned long) memblock_phys_mem_size()/1024/1024);
	start_kernel();

	while(1);
}

