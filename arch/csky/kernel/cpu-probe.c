// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/of.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <abi/reg_ops.h>
#include <linux/memblock.h>

static __init void setup_cpu_msa(void)
{
	if (memblock_start_of_DRAM() != (PHYS_OFFSET + CONFIG_RAM_BASE)) {
		pr_err("C-SKY: dts-DRAM doesn't fit .config: %x-%x.\n",
			memblock_start_of_DRAM(),
			PHYS_OFFSET + CONFIG_RAM_BASE);
		return;
	}

	mtcr_msa0(PHYS_OFFSET | 0xe);
	mtcr_msa1(PHYS_OFFSET | 0x6);
}

__init void cpu_dt_probe(void)
{
	setup_cpu_msa();
}

static int c_show(struct seq_file *m, void *v)
{
	seq_printf(m, "C-SKY CPU : %s\n", CSKYCPU_DEF_NAME);
	seq_printf(m, "revision  : 0x%08x\n", mfcr_cpuidrr());
	seq_printf(m, "ccr reg   : 0x%08x\n", mfcr_ccr());
	seq_printf(m, "ccr2 reg  : 0x%08x\n", mfcr_ccr2());
	seq_printf(m, "hint reg  : 0x%08x\n", mfcr_hint());
	seq_printf(m, "msa0 reg  : 0x%08x\n", mfcr_msa0());
	seq_printf(m, "msa1 reg  : 0x%08x\n", mfcr_msa1());
	seq_printf(m, "\n");
#ifdef CSKY_ARCH_VERSION
	seq_printf(m, "arch-version : %s\n", CSKY_ARCH_VERSION);
	seq_printf(m, "\n");
#endif
	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void c_stop(struct seq_file *m, void *v) {}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show,
};

