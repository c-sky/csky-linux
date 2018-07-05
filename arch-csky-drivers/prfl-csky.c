// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ptrace.h>

static unsigned int hpcr;
int proc_hpcr(struct ctl_table *table, int write,
	      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
	asm volatile ("cprcr  %0, <0, 0x0>\n":"=r"(hpcr));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
	asm volatile ("cpwcr  %0, <0, 0x0>\n"::"r"(hpcr));

	pr_info("%s, %x\n", __func__, hpcr);

	return ret;
}

static unsigned int hpspr;
int proc_hpspr(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
	asm volatile ("cprcr  %0, <0, 0x1>\n":"=r"(hpspr));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
	asm volatile ("cpwcr  %0, <0, 0x1>\n"::"r"(hpspr));

	pr_info("%s, %x\n", __func__, hpspr);

	return ret;
}

static unsigned int hpepr;
int proc_hpepr(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
	asm volatile ("cprcr  %0, <0, 0x2>\n":"=r"(hpepr));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
	asm volatile ("cpwcr  %0, <0, 0x2>\n"::"r"(hpepr));

	pr_info("%s, %x\n", __func__, hpepr);

	return ret;
}

static unsigned int hpsir;
int proc_hpsir(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
	asm volatile ("cprcr  %0, <0, 0x3>\n":"=r"(hpsir));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
	asm volatile ("cpwcr  %0, <0, 0x3>\n"::"r"(hpsir));

	pr_info("%s, %x\n", __func__, hpsir);

	return ret;
}

static unsigned int hpcc_lo;
int proc_hpcc_lo(struct ctl_table *table, int write,
		 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x0>\n":"=r"(hpcc_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int hpcc_hi;
int proc_hpcc_hi(struct ctl_table *table, int write,
		 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x1>\n":"=r"(hpcc_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cc_lo;
int proc_cc_lo(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x2>\n":"=r"(cc_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cc_hi;
int proc_cc_hi(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x3>\n":"=r"(cc_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int tic_lo;
int proc_tic_lo(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x4>\n":"=r"(tic_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int tic_hi;
int proc_tic_hi(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x5>\n":"=r"(tic_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_access_lo;
int proc_l1icache_access_lo(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x6>\n":"=r"(l1icache_access_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_access_hi;
int proc_l1icache_access_hi(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x7>\n":"=r"(l1icache_access_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_miss_lo;
int proc_l1icache_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x8>\n":"=r"(l1icache_miss_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_miss_hi;
int proc_l1icache_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x9>\n":"=r"(l1icache_miss_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_access_lo;
int proc_l1dcache_access_lo(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0xa>\n":"=r"(l1dcache_access_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_access_hi;
int proc_l1dcache_access_hi(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0xb>\n":"=r"(l1dcache_access_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_miss_lo;
int proc_l1dcache_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0xc>\n":"=r"(l1dcache_miss_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_miss_hi;
int proc_l1dcache_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0xd>\n":"=r"(l1dcache_miss_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_access_lo;
int proc_l2cache_access_lo(struct ctl_table *table, int write,
			   void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0xe>\n":"=r"(l2cache_access_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_access_hi;
int proc_l2cache_access_hi(struct ctl_table *table, int write,
			   void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0xf>\n":"=r"(l2cache_access_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_miss_lo;
int proc_l2cache_miss_lo(struct ctl_table *table, int write,
			 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x10>\n":"=r"(l2cache_miss_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_miss_hi;
int proc_l2cache_miss_hi(struct ctl_table *table, int write,
			 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x11>\n":"=r"(l2cache_miss_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int soft_counter_lo;
int proc_soft_counter_lo(struct ctl_table *table, int write,
		 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x1a>\n":"=r"(soft_counter_lo));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int soft_counter_hi;
int proc_soft_counter_hi(struct ctl_table *table, int write,
		 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	asm volatile ("cprgr  %0, <0, 0x1b>\n":"=r"(soft_counter_hi));

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static struct ctl_table prfl_tbl[] = {
	{
		.procname = "hpcr",
		.data = &hpcr,
		.maxlen = sizeof(hpcr),
		.mode = 0666,
		.proc_handler = &proc_hpcr
	},
	{
		.procname = "hpspr",
		.data = &hpspr,
		.maxlen = sizeof(hpspr),
		.mode = 0666,
		.proc_handler = &proc_hpspr
	},
	{
		.procname = "hpepr",
		.data = &hpepr,
		.maxlen = sizeof(hpepr),
		.mode = 0666,
		.proc_handler = &proc_hpepr
	},
	{
		.procname = "hpsir",
		.data = &hpsir,
		.maxlen = sizeof(hpsir),
		.mode = 0666,
		.proc_handler = &proc_hpsir
	},
	{
		.procname = "HardwarePfCycleCounter_lo",
		.data = &hpcc_lo,
		.maxlen = sizeof(hpcc_lo),
		.mode = 0666,
		.proc_handler = &proc_hpcc_lo
	},
	{
		.procname = "HardwarePfCycleCounter_hi",
		.data = &hpcc_hi,
		.maxlen = sizeof(hpcc_hi),
		.mode = 0666,
		.proc_handler = &proc_hpcc_hi
	},
	{
		.procname = "CycleCounter_lo",
		.data = &cc_lo,
		.maxlen = sizeof(cc_lo),
		.mode = 0666,
		.proc_handler = &proc_cc_lo
	},
	{
		.procname = "CycleCounter_hi",
		.data = &cc_hi,
		.maxlen = sizeof(cc_hi),
		.mode = 0666,
		.proc_handler = &proc_cc_hi
	},
	{
		.procname = "TotalInstructionCounter_lo",
		.data = &tic_lo,
		.maxlen = sizeof(tic_lo),
		.mode = 0666,
		.proc_handler = &proc_tic_lo
	},
	{
		.procname = "TotalInstructionCounter_hi",
		.data = &tic_hi,
		.maxlen = sizeof(tic_hi),
		.mode = 0666,
		.proc_handler = &proc_tic_hi
	},
	{
		.procname = "L1IcacheAccessCounter_lo",
		.data = &l1icache_access_lo,
		.maxlen = sizeof(l1icache_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l1icache_access_lo
	},
	{
		.procname = "L1IcacheAccessCounter_hi",
		.data = &l1icache_access_hi,
		.maxlen = sizeof(l1icache_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l1icache_access_hi
	},
	{
		.procname = "L1IcacheMissCounter_lo",
		.data = &l1icache_miss_lo,
		.maxlen = sizeof(l1icache_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l1icache_miss_lo
	},
	{
		.procname = "L1IcacheMissCounter_hi",
		.data = &l1icache_miss_hi,
		.maxlen = sizeof(l1icache_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l1icache_miss_hi
	},
	{
		.procname = "L1DcacheAccessCounter_lo",
		.data = &l1dcache_access_lo,
		.maxlen = sizeof(l1dcache_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_access_lo
	},
	{
		.procname = "L1DcacheAccessCounter_hi",
		.data = &l1dcache_access_hi,
		.maxlen = sizeof(l1dcache_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_access_hi
	},
	{
		.procname = "L1DcacheMissCounter_lo",
		.data = &l1dcache_miss_lo,
		.maxlen = sizeof(l1dcache_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_miss_lo
	},
	{
		.procname = "L1DcacheMissCounter_hi",
		.data = &l1dcache_miss_hi,
		.maxlen = sizeof(l1dcache_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_miss_hi
	},
	{
		.procname = "L2cacheAccessCounter_lo",
		.data = &l2cache_access_lo,
		.maxlen = sizeof(l2cache_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l2cache_access_lo
	},
	{
		.procname = "L2cacheAccessCounter_hi",
		.data = &l2cache_access_hi,
		.maxlen = sizeof(l2cache_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l2cache_access_hi
	},
	{
		.procname = "L2cacheMissCounter_lo",
		.data = &l2cache_miss_lo,
		.maxlen = sizeof(l2cache_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l2cache_miss_lo
	},
	{
		.procname = "L2cacheMissCounter_hi",
		.data = &l2cache_miss_hi,
		.maxlen = sizeof(l2cache_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l2cache_miss_hi
	},
	{
		.procname = "SoftwareCounter_lo",
		.data = &soft_counter_lo,
		.maxlen = sizeof(soft_counter_lo),
		.mode = 0666,
		.proc_handler = &proc_soft_counter_lo
	},
	{
		.procname = "SoftwareCounter_hi",
		.data = &soft_counter_hi,
		.maxlen = sizeof(soft_counter_hi),
		.mode = 0666,
		.proc_handler = &proc_soft_counter_hi
	},
	{}
};

static struct ctl_table sysctl_table[2] = {
	{
	 .procname = "csky_prfl",
	 .mode = 0555,
	 .child = prfl_tbl},
	{}
};

static struct ctl_path sysctl_path[2] = {
	{.procname = "csky"},
	{}
};

static int __init csky_prfl_init(void)
{
	register_sysctl_paths(sysctl_path, sysctl_table);
	return 0;
}

arch_initcall(csky_prfl_init);

