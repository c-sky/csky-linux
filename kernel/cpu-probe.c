/*
 * Processor capabilities determination functions.
 *
 * Copyright (C) 2011  Hangzhou C-SKY Microsystems co.,ltd.
 * Copyright (C) 2011  Hu junshan (junshan_hu@c-sky.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/smp.h>
#include <linux/stddef.h>
#include <asm/cpu.h>

char __cpu_name[NR_CPUS][32];

static inline int  read_cpuid_reg(void)
{
	int __res;
	__asm__ __volatile__("mfcr %0,cr13\n\t"
			:"=r" (__res));
	return   __res;
}

static const char *cpuid_v1_model[] = {
	"10",
	"10E",
	"10S",
	"10ES",
	"20",
	"Reserved",
	"20S",
	"Reserved",
	"10M",
	"10ME",
	"10MS",
	"10MES",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	NULL
};

static inline void cpu_probe_ver1(struct cpuinfo_csky *c, unsigned int cpu)
{
	char *p = __cpu_name[cpu];

#if defined(CONFIG_CPU_HAS_FPU)
	c->fpu_id   = CPUID_FPU_V1;
#endif

	switch (c->processor_id[0] & 0xf0000000) {
	case CPUID_V1_FAMILY_CK500:
		c->cputype = CPU_CK500;
		sprintf(p, "CK5%s%s", cpuid_v1_model[(c->processor_id[0] >> 24) & 0xf],
				(c->fpu_id ? "F" : ""));
		break;
	case CPUID_V1_FAMILY_CK600:
		c->cputype = CPU_CK600;
		sprintf(p, "CK6%s%s", cpuid_v1_model[(c->processor_id[0] >> 24) & 0xf],
				c->fpu_id ? "F" : "");
		break;
	}

	switch ((c->processor_id[0] & 0x000f0000) >> 16) {
	case 1:
		c->cache_size |= CPUID_DCACHE_2K;
		break;
	case 2:
		c->cache_size |= CPUID_DCACHE_4K;
		break;
	case 3:
		c->cache_size |= CPUID_DCACHE_8K;
		break;
	case 4:
		c->cache_size |= CPUID_DCACHE_16K;
		break;
	case 5:
		c->cache_size |= CPUID_DCACHE_32K;
		break;
	case 6:
		c->cache_size |= CPUID_DCACHE_64K;
		break;
	default:
		c->cache_size |= CPUID_DCACHE_NONE;
		break;
	}

	switch ((c->processor_id[0] & 0x00f00000) >> 20) {
	case 1:
		c->cache_size |= CPUID_ICACHE_2K;
		break;
	case 2:
		c->cache_size |= CPUID_ICACHE_4K;
		break;
	case 3:
		c->cache_size |= CPUID_ICACHE_8K;
		break;
	case 4:
		c->cache_size |= CPUID_ICACHE_16K;
		break;
	case 5:
		c->cache_size |= CPUID_ICACHE_32K;
		break;
	case 6:
		c->cache_size |= CPUID_ICACHE_64K;
		break;
	default:
		c->cache_size |= CPUID_ICACHE_NONE;
		break;
	}
}

static inline void cpu_probe_ver2(struct cpuinfo_csky *c, unsigned int cpu)
{
	char *p = __cpu_name[cpu];

	switch (c->processor_id[0] & 0xf0000000) {
	case CPUID_V2_FAMILY_CK500:
		c->cputype = CPU_CK500;
		sprintf(p, "CK5%s%s%s%s%s%s",
				((c->processor_id[0] & CPUID_V2_MODEL_DS) ? "20" : "10"),
				((c->processor_id[0] & CPUID_V2_MODEL_DM) ? "E" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_SPM) ? "S" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_MMU) ? "M" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_FPU) ? "F" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_AXI) ? "A" : ""));
		if(c->processor_id[0] & CPUID_V2_MODEL_FPU) {
			c->fpu_id   = CPUID_FPU_V1;
		}
		break;
	case CPUID_V2_FAMILY_CK600:
		c->cputype = CPU_CK600;
		sprintf(p, "CK6%s%s%s%s%s%s",
				((c->processor_id[0] & CPUID_V2_MODEL_DS) ? "20" : "10"),
				((c->processor_id[0] & CPUID_V2_MODEL_DM) ? "E" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_SPM) ? "S" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_MMU) ? "M" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_FPU) ? "F" : ""),
				((c->processor_id[0] & CPUID_V2_MODEL_AXI) ? "A" : ""));
		if(c->processor_id[0] & CPUID_V2_MODEL_FPU) {
			c->fpu_id   = CPUID_FPU_V1;
		}
		break;
	}

	c->cache_size = CPUID_ICACHE_NONE | CPUID_DCACHE_NONE;
}

static inline void cpu_probe_ver3(struct cpuinfo_csky *c, unsigned int cpu)
{
	char *p = __cpu_name[cpu];

	c->processor_id[1] = read_cpuid_reg();
	c->processor_id[2] = read_cpuid_reg();
	c->processor_id[3] = read_cpuid_reg();

	switch (c->processor_id[0] & 0x03C00000) {
	case CPUID_V3_FAMILY_CK500:
		c->cputype = CPU_CK500;
		sprintf(p, "CK510%s%s%s%s",
				((c->processor_id[0] & CPUID_V3_MODEL_DM) ? "E" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_MMU) ? "M" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_FPU) ? "F" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_SPM) ? "S" : ""));
		if(c->processor_id[0] & CPUID_V3_MODEL_FPU) {
			c->fpu_id   = CPUID_FPU_V1;
		}
		break;
	case CPUID_V3_FAMILY_CK600:
		c->cputype = CPU_CK600;
		sprintf(p, "CK610%s%s%s%s",
				((c->processor_id[0] & CPUID_V3_MODEL_DM) ? "E" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_MMU) ? "M" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_FPU) ? "F" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_SPM) ? "S" : ""));
		if(c->processor_id[0] & CPUID_V3_MODEL_FPU) {
			c->fpu_id   = CPUID_FPU_V1;
		}
		break;
	case CPUID_V3_FAMILY_CK800:
		c->cputype = CPU_CK800;
		sprintf(p, "CK8%s%s%s%s%s%s%s%s",
				((c->processor_id[0] & CPUID_V3_CLASS_CK810) ? "10" : "03"),
				((c->processor_id[0] & CPUID_V3_MODEL_DM) ? "E" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_MMU) ? "M" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_FPU) ? "F" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_SPM) ? "S" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_MGU) ? "G" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_BCTM) ? "B" : ""),
				((c->processor_id[0] & CPUID_V3_MODEL_VDSP) ? "V" : ""));
		if(c->processor_id[0] & CPUID_V3_MODEL_FPU) {
			c->fpu_id   = CPUID_FPU_V2;
		}
		break;
	}

	switch ((c->processor_id[3] & 0x000000f0) >> 4) {
	case 1:
		c->cache_size |= CPUID_DCACHE_1K;
		break;
	case 2:
		c->cache_size |= CPUID_DCACHE_2K;
		break;
	case 3:
		c->cache_size |= CPUID_DCACHE_4K;
		break;
	case 4:
		c->cache_size |= CPUID_DCACHE_8K;
		break;
	case 5:
		c->cache_size |= CPUID_DCACHE_16K;
		break;
	case 6:
		c->cache_size |= CPUID_DCACHE_32K;
		break;
	case 7:
		c->cache_size |= CPUID_DCACHE_64K;
		break;
	case 8:
		c->cache_size |= CPUID_DCACHE_128K;
		break;
	case 9:
		c->cache_size |= CPUID_DCACHE_256K;
		break;
	default:
		c->cache_size |= CPUID_DCACHE_NONE;
		break;
	}

	switch (c->processor_id[3] & 0x0000000f) {
	case 1:
		c->cache_size |= CPUID_ICACHE_1K;
		break;
	case 2:
		c->cache_size |= CPUID_ICACHE_2K;
		break;
	case 3:
		c->cache_size |= CPUID_ICACHE_4K;
		break;
	case 4:
		c->cache_size |= CPUID_ICACHE_8K;
		break;
	case 5:
		c->cache_size |= CPUID_ICACHE_16K;
		break;
	case 6:
		c->cache_size |= CPUID_ICACHE_32K;
		break;
	case 7:
		c->cache_size |= CPUID_ICACHE_64K;
		break;
	case 8:
		c->cache_size |= CPUID_ICACHE_128K;
		break;
	case 9:
		c->cache_size |= CPUID_ICACHE_256K;
		break;
	default:
		c->cache_size |= CPUID_ICACHE_NONE;
		break;
	}
}

__init void cpu_probe(void)
{
	struct cpuinfo_csky *c = &current_cpu_data;
	unsigned int cpu = smp_processor_id();

	c->processor_id[0]	= CPUPID_UNKNOWN;
	c->fpu_id	= CPUID_FPU_NONE;
	c->cputype	= CPU_UNKNOWN;

	c->processor_id[0] = read_cpuid_reg();
	if(c->processor_id[0]) {
		switch (c->processor_id[0] & 0xf) {
		case CPUID_VER_1:
			cpu_probe_ver1(c, cpu);
			break;
		case CPUID_VER_2:
			cpu_probe_ver2(c, cpu);
			break;
		case CPUID_VER_3:
			cpu_probe_ver3(c, cpu);
			break;
		}
	}
}

__init void cpu_report(void)
{
	struct cpuinfo_csky *c = &current_cpu_data;

	printk(KERN_INFO "CPU revision is: 0x%08x (%s)\n",
			c->processor_id[0], __cpu_name[smp_processor_id()]);
#if defined(CONFIG_CPU_HAS_FPU)
	if(c->fpu_id) {
		printk(KERN_INFO "FPU revision is: %s\n",
				(c->fpu_id == CPUID_FPU_V1 ? "FPU(V1)" : "VFP(V2)"));
	}
#endif
}


