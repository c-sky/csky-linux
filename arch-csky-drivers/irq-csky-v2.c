// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/module.h>
#include <linux/irqdomain.h>
#include <linux/irqchip.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/traps.h>
#include <asm/reg_ops.h>
#include <asm/smp.h>

static void __iomem *INTCG_base;
static void __iomem *INTCL_base;

#define COMM_IRQ_BASE	32

#define INTCG_SIZE	0x8000
#define INTCL_SIZE	0x1000
#define INTC_SIZE	INTCL_SIZE*nr_cpu_ids + INTCG_SIZE

#define INTCG_ICTLR	0x0
#define INTCG_CICFGR	0x100
#define INTCG_CIDSTR	0x1000

#define INTCL_PICTLR	0x0
#define INTCL_SIGR	0x60
#define INTCL_RDYIR	0x6c
#define INTCL_SENR	0xa0
#define INTCL_CENR	0xa4
#define INTCL_CACR	0xb4

#define INTC_IRQS	256

#define INTC_ICR_AVE	BIT(31)

DEFINE_PER_CPU(void __iomem *, intcl_reg);

static void csky_irq_v2_handler(struct pt_regs *regs)
{
	static void __iomem	*reg_base;
	irq_hw_number_t		hwirq;

	reg_base = *this_cpu_ptr(&intcl_reg);

	hwirq = readl_relaxed(reg_base + INTCL_RDYIR);
	handle_domain_irq(NULL, hwirq, regs);
}

static void csky_irq_v2_enable(struct irq_data *d)
{
	static void __iomem	*reg_base;

	reg_base = *this_cpu_ptr(&intcl_reg);

	writel_relaxed(d->hwirq, reg_base + INTCL_SENR);
}

static void csky_irq_v2_disable(struct irq_data *d)
{
	static void __iomem	*reg_base;

	reg_base = *this_cpu_ptr(&intcl_reg);

	writel_relaxed(d->hwirq, reg_base + INTCL_CENR);
}

static void csky_irq_v2_eoi(struct irq_data *d)
{
	static void __iomem	*reg_base;

	reg_base = *this_cpu_ptr(&intcl_reg);

	writel_relaxed(d->hwirq, reg_base + INTCL_CACR);
}

#ifdef CONFIG_SMP
static int csky_irq_set_affinity(struct irq_data *d,
				 const struct cpumask *mask_val,
				 bool force)
{
	unsigned int cpu;

	if (!force)
		cpu = cpumask_any_and(mask_val, cpu_online_mask);
	else
		cpu = cpumask_first(mask_val);

	if (cpu >= nr_cpu_ids)
		return -EINVAL;

	/* Enable interrupt destination */
	cpu |= BIT(31);

	writel_relaxed(cpu, INTCG_base + INTCG_CIDSTR + (4*(d->hwirq - COMM_IRQ_BASE)));

	irq_data_update_effective_affinity(d, cpumask_of(cpu));

	return IRQ_SET_MASK_OK_DONE;
}
#endif

static struct irq_chip csky_irq_chip = {
	.name           = "C-SKY SMP Intc V2",
	.irq_eoi	= csky_irq_v2_eoi,
	.irq_enable	= csky_irq_v2_enable,
	.irq_disable	= csky_irq_v2_disable,
#ifdef CONFIG_SMP
	.irq_set_affinity = csky_irq_set_affinity,
#endif
};

static int csky_irqdomain_map(struct irq_domain *d, unsigned int irq,
			      irq_hw_number_t hwirq)
{
	if(hwirq < COMM_IRQ_BASE) {
		irq_set_percpu_devid(irq);
		irq_set_chip_and_handler(irq, &csky_irq_chip, handle_percpu_irq);
	} else
		irq_set_chip_and_handler(irq, &csky_irq_chip, handle_fasteoi_irq);

	return 0;
}

static const struct irq_domain_ops csky_irqdomain_ops = {
	.map	= csky_irqdomain_map,
	.xlate	= irq_domain_xlate_onecell,
};

#ifdef CONFIG_SMP
static void csky_irq_v2_send_ipi(const unsigned long *mask, unsigned long irq)
{
	static void __iomem	*reg_base;

	reg_base = *this_cpu_ptr(&intcl_reg);

	/*
	 * INTCL_SIGR[3:0] INTID
	 * INTCL_SIGR[8:15] CPUMASK
	 */
	writel_relaxed((*mask) << 8 | irq, reg_base + INTCL_SIGR);
}
#endif

static int __init
csky_intc_v2_init(struct device_node *node, struct device_node *parent)
{
	struct irq_domain *root_domain;
	int cpu;

	if (parent)
		return 0;

	if (INTCG_base == NULL) {
		INTCG_base = ioremap(mfcr("cr<31, 14>"), INTC_SIZE);
		if (INTCG_base == NULL)
			return -EIO;

		INTCL_base = INTCG_base + INTCG_SIZE;

		writel_relaxed(BIT(0), INTCG_base + INTCG_ICTLR);
	}

	root_domain = irq_domain_add_linear(node, INTC_IRQS,
					&csky_irqdomain_ops, NULL);
	if (!root_domain)
		return -ENXIO;

	irq_set_default_host(root_domain);

	/* for every cpu */
	for_each_present_cpu(cpu) {
		per_cpu(intcl_reg, cpu) = INTCL_base + (INTCL_SIZE * cpu);
		writel_relaxed(BIT(0), per_cpu(intcl_reg, cpu) + INTCL_PICTLR);
	}

	set_handle_irq(&csky_irq_v2_handler);

#ifdef CONFIG_SMP
	set_send_ipi(&csky_irq_v2_send_ipi);
#endif

	return 0;
}
IRQCHIP_DECLARE(csky_intc_v2, "csky,intc-v2", csky_intc_v2_init);

