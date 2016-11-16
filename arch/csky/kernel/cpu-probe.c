#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/cpu.h>

static char __cpu_name[NR_CPUS][64];

static inline int  read_cpuid_reg(void)
{
	int __res;
	__asm__ __volatile__("mfcr %0,cr13\n\t"
			:"=r" (__res));
	return   __res;
}

static inline void cpu_probe_ver2(struct cpuinfo_csky *c, unsigned int cpu)
{
	char *p = __cpu_name[cpu];

	switch (c->processor_id[0] & 0xf0000000) {
	case CPUID_V2_FAMILY_CK600:
		sprintf(p, "CK610%s%s%s",
				((c->processor_id[0] & CPUID_V2_MODEL_MMU) ? "-MMU" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_FPU) ? "-FPU" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_AXI) ? "-AXI" : ""));
		break;
	}
}

static inline void cpu_probe_ver3(struct cpuinfo_csky *c, unsigned int cpu)
{
	char *p = __cpu_name[cpu];

	c->processor_id[1] = read_cpuid_reg();
	c->processor_id[2] = read_cpuid_reg();
	c->processor_id[3] = read_cpuid_reg();

	switch (c->processor_id[0] & 0x03C00000) {
	case CPUID_V3_FAMILY_CK600:
		break;
	case CPUID_V3_FAMILY_CK800:
		sprintf(p, "CK8%s%s%s%s%s%s, Cache I/D:%dK/%dK",
				((c->processor_id[0] & CPUID_V3_CLASS_CK810) ? "10" : "07"),
				((c->processor_id[0] & CPUID_V3_MODEL_DM) ? "-DSP" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_MMU) ? "-MMU" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_FPU) ? "-FPU" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_BCTM) ? "-BCTM" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_VDSP) ? "-VDSP" : ""),
				1 <<  ((c->processor_id[3] & 0xf)-1),
				1 << (((c->processor_id[3] & 0xf0)>>4)-1));
		break;
	}
}

__init void cpu_probe(void)
{
	struct cpuinfo_csky *c = &current_cpu_data;
	unsigned int cpu = smp_processor_id();

	c->processor_id[0]	= CPUPID_UNKNOWN;

	c->processor_id[0] = read_cpuid_reg();
	if(c->processor_id[0]) {
		switch (c->processor_id[0] & 0xf) {
		case CPUID_VER_2:
			cpu_probe_ver2(c, cpu);
			break;
		case CPUID_VER_3:
			cpu_probe_ver3(c, cpu);
			break;
		}
	}

	printk(KERN_INFO "C-SKY CPU revision is: 0x%08x (%s)\n",
		c->processor_id[0], __cpu_name[smp_processor_id()]);
}

static int show_cpuinfo(struct seq_file *m, void *v)
{
	unsigned long n = (unsigned long) v - 1;
	char *fpu;
	u_long clockfreq;
	struct cpuinfo_csky * c=&cpu_data[n];
	fpu = "none";

	seq_printf(m, "Processor\t: %ld\n", n);
	seq_printf(m, "CPU\t\t: %s(0x%8x)\n", __cpu_name[n], c->processor_id[0]);

	/*
	 * The fiducial operation declt + bf need 2 cycle. So calculate CPU clock
	 *  need to multiply 2.
	 */
	clockfreq = (loops_per_jiffy*HZ)*2;
	seq_printf(m,
			"Clocking\t: %lu.%1luMHz\n"
			"BogoMips\t: %lu.%02lu\n",
			clockfreq / 1000000, (clockfreq / 10000) % 100,
			(loops_per_jiffy * HZ) / 500000, ((loops_per_jiffy * HZ) / 5000) % 100);

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	unsigned long i = *pos;

	return i < NR_CPUS ? (void *) (i + 1) : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return c_start(m, pos);
}

static void c_stop(struct seq_file *m, void *v)
{
}

struct seq_operations cpuinfo_op = {
start:	c_start,
	next:	c_next,
	stop:	c_stop,
	show:	show_cpuinfo,
};

