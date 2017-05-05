#include <linux/of.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <hal/reg_ops.h>

#define CCR_MP		(1 << 0)
#define CCR_IE		(1 << 2)
#define CCR_DE		(1 << 3)
#define CCR_WB		(1 << 4)
#define CCR_RS		(1 << 5)
#define CCR_Z		(1 << 6)
#define CCR_BE		(1 << 7)
#define CCR_BTB		(1 << 11)
#define CCR_WA		(1 << 12)
#define CCR_EV2		(1 << 13)
#define CCR_WBR		(1 << 14)
#define CCR_DLE		(1 << 15)

#define HINT_BURST	(1 << 1)
#define HINT_PLD	(1 << 2)
#define HINT_MB		(1 << 3)
#define HINT_LRU	(1 << 4)
#define HINT_IPLD	(1 << 8)

static struct {
	u32 ccr;
	u32 hint;
	u32 cpuid;
} cpu_feature;

char cpu_name[8] = "CKxxx";

static __init void setup_ccr_hint(struct device_node *cpu)
{
	cpu_feature.ccr = CCR_MP;
	cpu_feature.hint = 0;

	if (of_property_read_bool(cpu, "ccr_ie"))
		cpu_feature.ccr |= CCR_IE;

	if (of_property_read_bool(cpu, "ccr_de"))
		cpu_feature.ccr |= CCR_DE;

	if (of_property_read_bool(cpu, "ccr_wb"))
		cpu_feature.ccr |= CCR_WB;

	if (of_property_read_bool(cpu, "ccr_rs"))
		cpu_feature.ccr |= CCR_RS;

	if (of_property_read_bool(cpu, "ccr_z"))
		cpu_feature.ccr |= CCR_Z;

	if (of_property_read_bool(cpu, "ccr_be"))
		cpu_feature.ccr |= CCR_BE;

	if (of_property_read_bool(cpu, "ccr_btb"))
		cpu_feature.ccr |= CCR_BTB;

	if (of_property_read_bool(cpu, "ccr_wa"))
		cpu_feature.ccr |= CCR_WA;

	if (of_property_read_bool(cpu, "ccr_wbr"))
		cpu_feature.ccr |= CCR_WBR;

	if (of_property_read_bool(cpu, "ccr_dle"))
		cpu_feature.ccr |= CCR_DLE;

	if (of_property_read_bool(cpu, "hint_burst"))
		cpu_feature.hint |= HINT_BURST;

	if (of_property_read_bool(cpu, "hint_pld"))
		cpu_feature.hint |= HINT_PLD;

	if (of_property_read_bool(cpu, "hint_mb"))
		cpu_feature.hint |= HINT_MB;

	if (of_property_read_bool(cpu, "hint_lru"))
		cpu_feature.hint |= HINT_LRU;

	if (of_property_read_bool(cpu, "hint_ipld"))
		cpu_feature.hint |= HINT_IPLD;
}

static __init void setup_cpu_name(struct device_node *cpu)
{
	cpu_feature.cpuid = mfcr_cpuidrr();

	if (of_device_is_compatible(cpu, "csky,ck610"))
		sprintf(cpu_name, "CK610");
	if (of_device_is_compatible(cpu, "csky,ck810"))
		sprintf(cpu_name, "CK810");
	if (of_device_is_compatible(cpu, "csky,ck807"))
		sprintf(cpu_name, "CK807");

	cache_op_all(DATA_CACHE|CACHE_CLR);
	mtcr_hint(cpu_feature.hint);
	mtcr_ccr(cpu_feature.ccr);
	cache_op_all(DATA_CACHE|CACHE_CLR);
	return;
}

__init void cpu_dt_probe(void)
{
	struct device_node *cpu;

	cpu = (struct device_node *) of_find_node_by_type(NULL, "cpu");
	if (!cpu) {
		pr_info("C-SKY Err: None cpu described in DeviceTree!");
		return;
	}

	setup_ccr_hint(cpu);
	setup_cpu_name(cpu);
}

static int c_show(struct seq_file *m, void *v)
{
	seq_printf(m, "C-SKY CPU revision is: 0x%08x (%s)\n",
				cpu_feature.cpuid, cpu_name);
	seq_printf(m, "ccr reg        : 0x%x\n", cpu_feature.ccr);
	seq_printf(m, "hint reg       : 0x%x\n", cpu_feature.hint);
	seq_printf(m, "\n");
	seq_printf(m, "ccr_ie      (2): %s\n", cpu_feature.ccr & CCR_IE    ? "enable":"disabled");
	seq_printf(m, "ccr_de      (3): %s\n", cpu_feature.ccr & CCR_DE    ? "enable":"disabled");
	seq_printf(m, "ccr_wb      (4): %s\n", cpu_feature.ccr & CCR_WB    ? "enable":"disabled");
	seq_printf(m, "ccr_rs      (5): %s\n", cpu_feature.ccr & CCR_RS    ? "enable":"disabled");
	seq_printf(m, "ccr_z       (6): %s\n", cpu_feature.ccr & CCR_Z     ? "enable":"disabled");
	seq_printf(m, "ccr_be      (7): %s\n", cpu_feature.ccr & CCR_BE    ? "enable":"disabled");
	seq_printf(m, "ccr_btb    (11): %s\n", cpu_feature.ccr & CCR_BTB   ? "enable":"disabled");
	seq_printf(m, "ccr_wa     (12): %s\n", cpu_feature.ccr & CCR_WA    ? "enable":"disabled");
	seq_printf(m, "ccr_wbr    (14): %s\n", cpu_feature.ccr & CCR_WBR   ? "enable":"disabled");
	seq_printf(m, "ccr_dle    (15): %s\n", cpu_feature.ccr & CCR_DLE   ? "enable":"disabled");
	seq_printf(m, "\n");
	seq_printf(m, "hint_burst  (1): %s\n", cpu_feature.hint& HINT_BURST? "enable":"disabled");
	seq_printf(m, "hint_pld    (2): %s\n", cpu_feature.hint& HINT_PLD  ? "enable":"disabled");
	seq_printf(m, "hint_mb     (3): %s\n", cpu_feature.hint& HINT_MB   ? "enable":"disabled");
	seq_printf(m, "hint_lru    (4): %s\n", cpu_feature.hint& HINT_LRU  ? "enable":"disabled");
	seq_printf(m, "hint_ipld   (8): %s\n", cpu_feature.hint& HINT_IPLD ? "enable":"disabled");
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

static void c_stop(struct seq_file *m, void *v)
{
}

const struct seq_operations cpuinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show,
};

