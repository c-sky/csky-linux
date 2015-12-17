/*
 * linux/arch/csky/mm/nommu.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2011 C-SKY Microsystems co.,ltd.  All rights reserved.
 * Copyright (C) 2011 Hu Junshan(junshan_hu@c-sky.com)
 */
#include <linux/bug.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <linux/proc_fs.h>
#include <linux/pfn.h>

#include <asm/setup.h>
#include <asm/cachectl.h>
#include <asm/dma.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/mmu_context.h>
#include <asm/sections.h>
#include <asm/csky.h>

extern struct boot_mem_map boot_mem_map;
extern unsigned int phy_ramstart, phy_ramend;
EXPORT_SYMBOL(phy_ramend);
EXPORT_SYMBOL(phy_ramstart);

void __init paging_init(void)
{
	unsigned long max_zone_pfns[MAX_NR_ZONES];
	unsigned long lastpfn;

#ifdef CONFIG_ZONE_DMA
	max_zone_pfns[ZONE_DMA] = MAX_DMA_PFN;
#endif
#ifdef CONFIG_ZONE_DMA32
	max_zone_pfns[ZONE_DMA32] = MAX_DMA32_PFN;
#endif
	max_zone_pfns[ZONE_NORMAL] = max_low_pfn;
	lastpfn = max_low_pfn;

	free_area_init_nodes(max_zone_pfns);
}

/*
 *
 */
void __init setup_memory(void)
{
	int i, oldnrmap;
	unsigned long tmp_start, tmp_end;

	oldnrmap = boot_mem_map.nr_map;
	if(0 == oldnrmap)
	{
		add_memory_region(phy_ramstart,
		     phy_ramend - phy_ramstart, BOOT_MEM_RAM);
		return;
	}
	boot_mem_map.nr_map = 0;
	for (i = 0; i < oldnrmap; i++) {
		unsigned long start, end;

		if (boot_mem_map.map[i].type != BOOT_MEM_RAM)
			continue;

		tmp_start = phy_ramstart;
		tmp_end = phy_ramend;
		start = boot_mem_map.map[i].addr;
		end = boot_mem_map.map[i].addr + boot_mem_map.map[i].size;

		if ((end <= phy_ramstart) || (start >= phy_ramend))
			continue;

		if( start > phy_ramstart)
			tmp_start = start;
		if(end < phy_ramend)
			tmp_end = end;
		add_memory_region(tmp_start, tmp_end - tmp_start, BOOT_MEM_RAM);
	}
}
