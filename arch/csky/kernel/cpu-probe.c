// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/of.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/memblock.h>

#include <abi/reg_ops.h>

static __init void setup_cpu_msa(void)
{
	if (memblock_start_of_DRAM() != (PHYS_OFFSET + CONFIG_RAM_BASE)) {
		pr_err("C-SKY: dts-DRAM doesn't fit .config: %x-%x.\n",
			memblock_start_of_DRAM(),
			PHYS_OFFSET + CONFIG_RAM_BASE);
		return;
	}

	mtcr_msa0(PHYS_OFFSET | 0xe);
	mtcr_msa1(PHYS_OFFSET | 0x26);
}

__init void cpu_dt_probe(void)
{
	setup_cpu_msa();
}

static void percpu_print(void *arg)
{
	struct seq_file *m = (struct seq_file *)arg;

	seq_printf(m, "processor       : %d\n", smp_processor_id());
	seq_printf(m, "C-SKY CPU model : %s\n", CSKYCPU_DEF_NAME);

	/* Read 4 times to get all the cpuid info */
	seq_printf(m, "product info[0] : 0x%08x\n", mfcr("cr13"));
	seq_printf(m, "product info[1] : 0x%08x\n", mfcr("cr13"));
	seq_printf(m, "product info[2] : 0x%08x\n", mfcr("cr13"));
	seq_printf(m, "product info[3] : 0x%08x\n", mfcr("cr13"));

	seq_printf(m, "mpid reg        : 0x%08x\n", mfcr("cr30"));
	seq_printf(m, "ccr reg         : 0x%08x\n", mfcr("cr18"));
	seq_printf(m, "ccr2 reg        : 0x%08x\n", mfcr_ccr2());
	seq_printf(m, "hint reg        : 0x%08x\n", mfcr_hint());
	seq_printf(m, "msa0 reg        : 0x%08x\n", mfcr_msa0());
	seq_printf(m, "msa1 reg        : 0x%08x\n", mfcr_msa1());
	seq_printf(m, "\n");
}

static int c_show(struct seq_file *m, void *v)
{
	int cpu;

	for_each_online_cpu(cpu)
		smp_call_function_single(cpu, percpu_print, m, true);

#ifdef CSKY_ARCH_VERSION
	seq_printf(m, "\n");
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

