#include <linux/of.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <hal/reg_ops.h>

static struct {
	u32 ccr;
	u32 hint;
	u32 cpuid;
} cpu_feature;

char cpu_name[8] = "CKxxx";

static __init void setup_ccr_hint(struct device_node *cpu)
{
	if (of_property_read_u32(cpu, "ccr", &cpu_feature.ccr))
		return;

	if (of_property_read_u32(cpu, "hint", &cpu_feature.hint))
		return;

	cache_op_all(DATA_CACHE|CACHE_CLR);
	mtcr_hint(cpu_feature.hint);
	mtcr_ccr(cpu_feature.ccr);
	cache_op_all(DATA_CACHE|CACHE_CLR);
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
	seq_printf(m, "C-SKY CPU : %s\n", cpu_name);
	seq_printf(m, "revision  : 0x%08x\n", cpu_feature.cpuid);
	seq_printf(m, "ccr reg   : 0x%08x\n", cpu_feature.ccr);
	seq_printf(m, "hint reg  : 0x%08x\n", cpu_feature.hint);
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

