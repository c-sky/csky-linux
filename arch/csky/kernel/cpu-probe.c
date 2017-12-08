#include <linux/of.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <hal/reg_ops.h>
#include <linux/memblock.h>

char cpu_name[32] = CSKYCPU_DEF_NAME;

static __init void setup_ccr_hint(void)
{
	unsigned int ccr = 0;
	unsigned int ccr2 = 0;
	unsigned int hint = 0;
	struct device_node *cpu;

	cpu = of_find_node_by_type(NULL, "cpu");
	if (cpu) {
		if (of_property_read_u32(cpu, "ccr", &ccr)) ccr = 0;
		if (of_property_read_u32(cpu, "ccr2", &ccr2)) ccr2 = 0;
		if (of_property_read_u32(cpu, "hint", &hint)) hint = 0;
	}

	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 1);
	if (ccr) mtcr_ccr(ccr);
	if (ccr2) mtcr_ccr2(ccr2);
	if (hint) mtcr_hint(hint);
	if (ccr2 & 0x8) cache_op_l2enable();
	cache_op_all(INS_CACHE|DATA_CACHE|CACHE_CLR|CACHE_INV, 1);
}

#define MSA_MASK 0xe0000000

static __init void setup_cpu_msa(void)
{
	unsigned int tmp;

	tmp = memblock_start_of_DRAM();

	if (tmp != (PHYS_OFFSET + CONFIG_RAM_BASE)) {
		pr_err("C-SKY: start of DRAM doesn't defconfig: %x-%x.\n",
		       tmp, PHYS_OFFSET + CONFIG_RAM_BASE);
		return;
	}

	if ((tmp & MSA_MASK) != (mfcr_msa0() & MSA_MASK)) {
		pr_err("C-SKY: start of DRAM doesn't fit MMU MSA0: %x-%x.\n",
			mfcr_msa0(), tmp);
		return;
	}

	tmp = tmp & MSA_MASK;
	mtcr_msa0(tmp | 0xe);
	mtcr_msa1(tmp | 0x6);
}

__init void cpu_dt_probe(void)
{
	setup_cpu_msa();
	setup_ccr_hint();
}

static int c_show(struct seq_file *m, void *v)
{
	seq_printf(m, "C-SKY CPU : %s\n", cpu_name);
	seq_printf(m, "revision  : 0x%08x\n", mfcr_cpuidrr());
	seq_printf(m, "ccr reg   : 0x%08x\n", mfcr_ccr());
	seq_printf(m, "ccr2 reg  : 0x%08x\n", mfcr_ccr2());
	seq_printf(m, "hint reg  : 0x%08x\n", mfcr_hint());
	seq_printf(m, "msa0 reg  : 0x%08x\n", mfcr_msa0());
	seq_printf(m, "msa1 reg  : 0x%08x\n", mfcr_msa1());
	seq_printf(m, "\n");

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

