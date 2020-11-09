// SPDX-License-Identifier: GPL-2.0

#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ptrace.h>

static unsigned int cpu_setting = 0;
int proc_cpu_setting(struct ctl_table *table, int write,
	      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	pr_info("%s, %x\n", __func__, cpu_setting);

	return ret;
}

static unsigned int hpcr;
static void proc_hpcr_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x0>\n":"=r"(hpcr));
}

static void proc_hpcr_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x0>\n"::"r"(hpcr));
}

int proc_hpcr(struct ctl_table *table, int write,
	      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpcr_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpcr_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpcr);

	return ret;
}

static unsigned int hpspr;
static void proc_hpspr_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x1>\n":"=r"(hpspr));
}

static void proc_hpspr_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x1>\n"::"r"(hpspr));
}

int proc_hpspr(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpspr_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpspr_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpspr);

	return ret;
}

static unsigned int hpepr;
static void proc_hpepr_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x2>\n":"=r"(hpepr));
}

static void proc_hpepr_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x2>\n"::"r"(hpepr));
}

int proc_hpepr(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpepr_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpepr_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpepr);

	return ret;
}

static unsigned int hpsir;
static void proc_hpsir_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x3>\n":"=r"(hpsir));
}

static void proc_hpsir_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x3>\n"::"r"(hpsir));
}
int proc_hpsir(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpsir_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpsir_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpsir);

	return ret;
}

static unsigned int hpcntenr;
static void proc_hpcntenr_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x4>\n":"=r"(hpcntenr));
}

static void proc_hpcntenr_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x4>\n"::"r"(hpcntenr));
}
int proc_hpcntenr(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpcntenr_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpcntenr_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpcntenr);

	return ret;
}

static unsigned int hpintenr;
static void proc_hpintenr_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x5>\n":"=r"(hpintenr));
}

static void proc_hpintenr_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x5>\n"::"r"(hpintenr));
}
int proc_hpintenr(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpintenr_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpintenr_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpintenr);

	return ret;
}

static unsigned int hpofsr;
static void proc_hpofsr_read(void *val)
{
	asm volatile ("cprcr  %0, <0, 0x6>\n":"=r"(hpofsr));
}

static void proc_hpofsr_write(void *val)
{
	asm volatile ("cpwcr  %0, <0, 0x6>\n"::"r"(hpofsr));
}

int proc_hpofsr(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (!write)
		smp_call_function_single(cpu_setting, proc_hpofsr_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	if (write)
		smp_call_function_single(cpu_setting, proc_hpofsr_write, NULL, 1);

	pr_info("%s, %x\n", __func__, hpofsr);

	return ret;
}

static unsigned int hpcc_lo;
static void proc_hpcc_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x0>\n":"=r"(hpcc_lo));
}
int proc_hpcc_lo(struct ctl_table *table, int write,
		 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_hpcc_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int hpcc_hi;
static void proc_hpcc_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1>\n":"=r"(hpcc_hi));
}
int proc_hpcc_hi(struct ctl_table *table, int write,
		 void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_hpcc_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cc_lo;
static void proc_cc_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2>\n":"=r"(cc_lo));
}
int proc_cc_lo(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_cc_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cc_hi;
static void proc_cc_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x3>\n":"=r"(cc_hi));
}
int proc_cc_hi(struct ctl_table *table, int write,
	       void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_cc_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int tic_lo;
static void proc_tic_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x4>\n":"=r"(tic_lo));
}
int proc_tic_lo(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_tic_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int tic_hi;
static void proc_tic_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x5>\n":"=r"(tic_hi));
}
int proc_tic_hi(struct ctl_table *table, int write,
		void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_tic_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_access_lo;
static void proc_l1icache_access_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x6>\n":"=r"(l1icache_access_lo));
}
int proc_l1icache_access_lo(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1icache_access_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_access_hi;
static void proc_l1icache_access_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x7>\n":"=r"(l1icache_access_hi));
}
int proc_l1icache_access_hi(struct ctl_table *table, int write,
			    void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1icache_access_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_miss_lo;
static void proc_l1icache_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x8>\n":"=r"(l1icache_miss_lo));
}
int proc_l1icache_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1icache_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1icache_miss_hi;
static void proc_l1icache_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x9>\n":"=r"(l1icache_miss_hi));
}
int proc_l1icache_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1icache_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int iutlb_miss_counter_lo;
static void proc_iutlb_miss_counter_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x14>\n":"=r"(iutlb_miss_counter_lo));
}
int proc_iutlb_miss_counter_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_iutlb_miss_counter_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int iutlb_miss_counter_hi;
static void proc_iutlb_miss_counter_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x15>\n":"=r"(iutlb_miss_counter_hi));
}
int proc_iutlb_miss_counter_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_iutlb_miss_counter_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int dutlb_miss_counter_lo;
static void proc_dutlb_miss_counter_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x16>\n":"=r"(dutlb_miss_counter_lo));
}
int proc_dutlb_miss_counter_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_dutlb_miss_counter_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int dutlb_miss_counter_hi;
static void proc_dutlb_miss_counter_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x17>\n":"=r"(dutlb_miss_counter_hi));
}
int proc_dutlb_miss_counter_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_dutlb_miss_counter_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int jtlb_miss_counter_lo;
static void proc_jtlb_miss_counter_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x18>\n":"=r"(jtlb_miss_counter_lo));
}
int proc_jtlb_miss_counter_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_jtlb_miss_counter_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int jtlb_miss_counter_hi;
static void proc_jtlb_miss_counter_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x19>\n":"=r"(jtlb_miss_counter_hi));
}
int proc_jtlb_miss_counter_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_jtlb_miss_counter_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int soft_counter_lo;
static void proc_soft_counter_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1a>\n":"=r"(soft_counter_lo));
}
int proc_soft_counter_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_soft_counter_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int soft_counter_hi;
static void proc_soft_counter_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1b>\n":"=r"(soft_counter_hi));
}
int proc_soft_counter_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_soft_counter_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int cond_br_miss_lo;
static void proc_cond_br_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1c>\n":"=r"(cond_br_miss_lo));
}
int proc_cond_br_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_cond_br_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cond_br_miss_hi;
static void proc_cond_br_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1d>\n":"=r"(cond_br_miss_hi));
}
int proc_cond_br_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_cond_br_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cond_br_insn_lo;
static void proc_cond_br_insn_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1e>\n":"=r"(cond_br_insn_lo));
}
int proc_cond_br_insn_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_cond_br_insn_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int cond_br_insn_hi;
static void proc_cond_br_insn_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x1f>\n":"=r"(cond_br_insn_hi));
}
int proc_cond_br_insn_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_cond_br_insn_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int indirect_br_miss_lo;
static void proc_indirect_br_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x20>\n":"=r"(indirect_br_miss_lo));
}
int proc_indirect_br_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_indirect_br_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int indirect_br_miss_hi;
static void proc_indirect_br_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x21>\n":"=r"(indirect_br_miss_hi));
}
int proc_indirect_br_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_indirect_br_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int indirect_br_insn_lo;
static void proc_indirect_br_insn_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x22>\n":"=r"(indirect_br_insn_lo));
}

int proc_indirect_br_insn_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_indirect_br_insn_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int indirect_br_insn_hi;
static void proc_indirect_br_insn_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x23>\n":"=r"(indirect_br_insn_hi));
}

int proc_indirect_br_insn_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_indirect_br_insn_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int lsu_spec_fail_lo;
static void proc_lsu_spec_fail_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x24>\n":"=r"(lsu_spec_fail_lo));
}

int proc_lsu_spec_fail_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_lsu_spec_fail_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int lsu_spec_fail_hi;
static void proc_lsu_spec_fail_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x25>\n":"=r"(lsu_spec_fail_hi));
}

int proc_lsu_spec_fail_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_lsu_spec_fail_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int store_insn_lo;
static void proc_store_insn_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x26>\n":"=r"(store_insn_lo));
}

int proc_store_insn_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_store_insn_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int store_insn_hi;
static void proc_store_insn_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x27>\n":"=r"(store_insn_hi));
}

int proc_store_insn_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_store_insn_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int l1dcache_read_access_lo;
static void proc_l1dcache_read_access_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x28>\n":"=r"(l1dcache_read_access_lo));
}

int proc_l1dcache_read_access_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_read_access_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_read_access_hi;
static void proc_l1dcache_read_access_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x29>\n":"=r"(l1dcache_read_access_hi));
}

int proc_l1dcache_read_access_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_read_access_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int l1dcache_read_miss_lo;
static void proc_l1dcache_read_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2a>\n":"=r"(l1dcache_read_miss_lo));
}

int proc_l1dcache_read_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_read_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_read_miss_hi;
static void proc_l1dcache_read_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2b>\n":"=r"(l1dcache_read_miss_hi));
}

int proc_l1dcache_read_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_read_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int l1dcache_write_access_lo;
static void proc_l1dcache_write_access_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2c>\n":"=r"(l1dcache_write_access_lo));
}

int proc_l1dcache_write_access_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_write_access_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_write_access_hi;
static void proc_l1dcache_write_access_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2d>\n":"=r"(l1dcache_write_access_hi));
}

int proc_l1dcache_write_access_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_write_access_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int l1dcache_write_miss_lo;
static void proc_l1dcache_write_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2e>\n":"=r"(l1dcache_write_miss_lo));
}

int proc_l1dcache_write_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l1dcache_write_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l1dcache_write_miss_hi;
static void l1dcache_write_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x2f>\n":"=r"(l1dcache_write_miss_hi));
}

int proc_l1dcache_write_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, l1dcache_write_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int l2cache_read_access_lo;
static void proc_l2cache_read_access_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x30>\n":"=r"(l2cache_read_access_lo));
}

int proc_l2cache_read_access_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_read_access_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_read_access_hi;
static void proc_l2cache_read_access_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x31>\n":"=r"(l2cache_read_access_hi));
}

int proc_l2cache_read_access_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_read_access_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_read_miss_lo;
static void proc_l2cache_read_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x32>\n":"=r"(l2cache_read_miss_lo));
}

int proc_l2cache_read_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_read_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_read_miss_hi;
static void proc_l2cache_read_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x33>\n":"=r"(l2cache_read_miss_hi));
}

int proc_l2cache_read_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_read_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_write_access_lo;
static void proc_l2cache_write_access_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x34>\n":"=r"(l2cache_write_access_lo));
}

int proc_l2cache_write_access_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_write_access_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_write_access_hi;
static void proc_l2cache_write_access_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x35>\n":"=r"(l2cache_write_access_hi));
}

int proc_l2cache_write_access_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_write_access_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static unsigned int l2cache_write_miss_lo;
static void proc_l2cache_write_miss_lo_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x36>\n":"=r"(l2cache_write_miss_lo));
}

int proc_l2cache_write_miss_lo(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_write_miss_lo_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}

static unsigned int l2cache_write_miss_hi;
static void proc_l2cache_write_miss_hi_read(void *val)
{
	asm volatile ("cprgr  %0, <0, 0x37>\n":"=r"(l2cache_write_miss_hi));
}

int proc_l2cache_write_miss_hi(struct ctl_table *table, int write,
			  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	int ret;

	if (write)
		return 0;

	smp_call_function_single(cpu_setting, proc_l2cache_write_miss_hi_read, NULL, 1);

	ret = proc_douintvec(table, write, buffer, lenp, ppos);

	return ret;
}


static struct ctl_table prfl_tbl[] = {
	{
		.procname = "cpu_setting",
		.data = &cpu_setting,
		.maxlen = sizeof(cpu_setting),
		.mode = 0666,
		.proc_handler = &proc_cpu_setting
	},
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
		.procname = "hpcntenr",
		.data = &hpcntenr,
		.maxlen = sizeof(hpcntenr),
		.mode = 0666,
		.proc_handler = &proc_hpcntenr
	},
	{
		.procname = "hpintenr",
		.data = &hpintenr,
		.maxlen = sizeof(hpintenr),
		.mode = 0666,
		.proc_handler = &proc_hpintenr
	},
	{
		.procname = "hpofsr",
		.data = &hpofsr,
		.maxlen = sizeof(hpofsr),
		.mode = 0666,
		.proc_handler = &proc_hpofsr
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
		.procname = "IUTLBMissCounter_lo",
		.data = &iutlb_miss_counter_lo,
		.maxlen = sizeof(iutlb_miss_counter_lo),
		.mode = 0666,
		.proc_handler = &proc_iutlb_miss_counter_lo
	},
	{
		.procname = "IUTLBMissCounter_hi",
		.data = &iutlb_miss_counter_hi,
		.maxlen = sizeof(iutlb_miss_counter_hi),
		.mode = 0666,
		.proc_handler = &proc_iutlb_miss_counter_hi
	},
	{
		.procname = "DUTLBMissCounter_lo",
		.data = &dutlb_miss_counter_lo,
		.maxlen = sizeof(dutlb_miss_counter_lo),
		.mode = 0666,
		.proc_handler = &proc_dutlb_miss_counter_lo
	},
	{
		.procname = "DUTLBMissCounter_hi",
		.data = &dutlb_miss_counter_hi,
		.maxlen = sizeof(dutlb_miss_counter_hi),
		.mode = 0666,
		.proc_handler = &proc_dutlb_miss_counter_hi
	},
	{
		.procname = "JTLBMissCounter_lo",
		.data = &jtlb_miss_counter_lo,
		.maxlen = sizeof(jtlb_miss_counter_lo),
		.mode = 0666,
		.proc_handler = &proc_jtlb_miss_counter_lo
	},
	{
		.procname = "JTLBMissCounter_hi",
		.data = &jtlb_miss_counter_hi,
		.maxlen = sizeof(jtlb_miss_counter_hi),
		.mode = 0666,
		.proc_handler = &proc_jtlb_miss_counter_hi
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
	{
		.procname = "ConditionalBranchMispredictCounter_lo",
		.data = &cond_br_miss_lo,
		.maxlen = sizeof(cond_br_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_cond_br_miss_lo
	},
	{
		.procname = "ConditionalBranchMispredictCounter_hi",
		.data = &cond_br_miss_hi,
		.maxlen = sizeof(cond_br_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_cond_br_miss_hi
	},
	{
		.procname = "ConditionalBranchInstructionCounter_lo",
		.data = &cond_br_insn_lo,
		.maxlen = sizeof(cond_br_insn_lo),
		.mode = 0666,
		.proc_handler = &proc_cond_br_insn_lo
	},
	{
		.procname = "ConditionalBranchInstructionCounter_hi",
		.data = &cond_br_insn_hi,
		.maxlen = sizeof(cond_br_insn_hi),
		.mode = 0666,
		.proc_handler = &proc_cond_br_insn_hi
	},
	{
		.procname = "IndirectBranchMispredictCounter_lo",
		.data = &indirect_br_miss_lo,
		.maxlen = sizeof(indirect_br_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_indirect_br_miss_lo
	},
	{
		.procname = "IndirectBranchMispredictCounter_hi",
		.data = &indirect_br_miss_hi,
		.maxlen = sizeof(indirect_br_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_indirect_br_miss_hi
	},
	{
		.procname = "IndirectBranchInstructionCounter_lo",
		.data = &indirect_br_insn_lo,
		.maxlen = sizeof(indirect_br_insn_lo),
		.mode = 0666,
		.proc_handler = &proc_indirect_br_insn_lo
	},
	{
		.procname = "IndirectBranchInstructionCounter_hi",
		.data = &indirect_br_insn_hi,
		.maxlen = sizeof(indirect_br_insn_hi),
		.mode = 0666,
		.proc_handler = &proc_indirect_br_insn_hi
	},
	{
		.procname = "LSUSpecFailCounter_lo",
		.data = &lsu_spec_fail_lo,
		.maxlen = sizeof(lsu_spec_fail_lo),
		.mode = 0666,
		.proc_handler = &proc_lsu_spec_fail_lo
	},
	{
		.procname = "LSUSpecFailCounter_hi",
		.data = &lsu_spec_fail_hi,
		.maxlen = sizeof(lsu_spec_fail_hi),
		.mode = 0666,
		.proc_handler = &proc_lsu_spec_fail_hi
	},
	{
		.procname = "StoreInstructionCounter_lo",
		.data = &store_insn_lo,
		.maxlen = sizeof(store_insn_lo),
		.mode = 0666,
		.proc_handler = &proc_store_insn_lo
	},
	{
		.procname = "StoreInstructionCounter_hi",
		.data = &store_insn_hi,
		.maxlen = sizeof(store_insn_hi),
		.mode = 0666,
		.proc_handler = &proc_store_insn_hi
	},
	{
		.procname = "L1DcacheReadAccessCounter_lo",
		.data = &l1dcache_read_access_lo,
		.maxlen = sizeof(l1dcache_read_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_read_access_lo
	},
	{
		.procname = "L1DcacheReadAccessCounter_hi",
		.data = &l1dcache_read_access_hi,
		.maxlen = sizeof(l1dcache_read_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_read_access_hi
	},
	{
		.procname = "L1DcacheReadMissCounter_lo",
		.data = &l1dcache_read_miss_lo,
		.maxlen = sizeof(l1dcache_read_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_read_miss_lo
	},
	{
		.procname = "L1DcacheReadMissCounter_hi",
		.data = &l1dcache_read_miss_hi,
		.maxlen = sizeof(l1dcache_read_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_read_miss_hi
	},
	{
		.procname = "L1DcacheWriteAccessCounter_lo",
		.data = &l1dcache_write_access_lo,
		.maxlen = sizeof(l1dcache_write_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_write_access_lo
	},
	{
		.procname = "L1DcacheWriteAccessCounter_hi",
		.data = &l1dcache_write_access_hi,
		.maxlen = sizeof(l1dcache_write_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_write_access_hi
	},
	{
		.procname = "L1DcacheWriteMissCounter_lo",
		.data = &l1dcache_write_miss_lo,
		.maxlen = sizeof(l1dcache_write_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_write_miss_lo
	},
	{
		.procname = "L1DcacheWriteMissCounter_hi",
		.data = &l1dcache_write_miss_hi,
		.maxlen = sizeof(l1dcache_write_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l1dcache_write_miss_hi
	},
	{
		.procname = "L2cacheReadAccessCounter_lo",
		.data = &l2cache_read_access_lo,
		.maxlen = sizeof(l2cache_read_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l2cache_read_access_lo
	},
	{
		.procname = "L2cacheReadAccessCounter_hi",
		.data = &l2cache_read_access_hi,
		.maxlen = sizeof(l2cache_read_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l2cache_read_access_hi
	},
	{
		.procname = "L2cacheReadMissCounter_lo",
		.data = &l2cache_read_miss_lo,
		.maxlen = sizeof(l2cache_read_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l2cache_read_miss_lo
	},
	{
		.procname = "L2cacheReadMissCounter_hi",
		.data = &l2cache_read_miss_hi,
		.maxlen = sizeof(l2cache_read_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l2cache_read_miss_hi
	},
	{
		.procname = "L2cacheWriteAccessCounter_lo",
		.data = &l2cache_write_access_lo,
		.maxlen = sizeof(l2cache_write_access_lo),
		.mode = 0666,
		.proc_handler = &proc_l2cache_write_access_lo
	},
	{
		.procname = "L2cacheWriteAccessCounter_hi",
		.data = &l2cache_write_access_hi,
		.maxlen = sizeof(l2cache_write_access_hi),
		.mode = 0666,
		.proc_handler = &proc_l2cache_write_access_hi
	},
	{
		.procname = "L2cacheWriteMissCounter_lo",
		.data = &l2cache_write_miss_lo,
		.maxlen = sizeof(l2cache_write_miss_lo),
		.mode = 0666,
		.proc_handler = &proc_l2cache_write_miss_lo
	},
	{
		.procname = "L2cacheWriteMissCounter_hi",
		.data = &l2cache_write_miss_hi,
		.maxlen = sizeof(l2cache_write_miss_hi),
		.mode = 0666,
		.proc_handler = &proc_l2cache_write_miss_hi
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
