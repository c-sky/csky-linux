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

static void __iomem *INTCG_base;
static void __iomem *INTCL_base;

#define INTC_SIZE	0x10000
#define INTCG_SIZE	0x8000
#define INTCL_SIZE	0x1000

#define INTCG_ICTLR	0x0
#define INTCG_CICFGR	0x80
#define INTCG_CIDSTR	0x1000

#define INTCL_PICTLR	0x0
#define INTCL_RDYIR	0x6c
#define INTCL_SENR	0xa0
#define INTCL_CENR	0xa4
#define INTCL_CACR	0xb4

#define INTC_IRQS	128

#define INTC_ICR_AVE	BIT(31)

#define COMM_IRQ_START	32

struct csky_irq_v2_data {
	struct irq_chip		chip;
	struct irq_domain	*domain;
	void __iomem		*intcl_reg;
	int			cpuid;
	char			name[20];
};
DEFINE_PER_CPU(struct csky_irq_v2_data, csky_irq_v2_data);

static void csky_irq_v2_handler(struct pt_regs *regs)
{
	struct irq_domain	*domain;
	static void __iomem	*reg_base;
	irq_hw_number_t		irq;

	domain = per_cpu(csky_irq_v2_data, smp_processor_id()).domain;
	reg_base = per_cpu(csky_irq_v2_data, smp_processor_id()).intcl_reg;

	irq = readl_relaxed(reg_base + INTCL_RDYIR) - COMM_IRQ_START;
	handle_domain_irq(domain, irq, regs);
}

static void csky_irq_v2_enable(struct irq_data *d)
{
	struct csky_irq_v2_data *data = irq_data_get_irq_chip_data(d);

	writel_relaxed(d->hwirq + COMM_IRQ_START, data->intcl_reg + INTCL_SENR);
}

static void csky_irq_v2_disable(struct irq_data *d)
{
	struct csky_irq_v2_data *data = irq_data_get_irq_chip_data(d);

	writel_relaxed(d->hwirq + COMM_IRQ_START, data->intcl_reg + INTCL_CENR);
}

static void csky_irq_v2_eoi(struct irq_data *d)
{
	struct csky_irq_v2_data *data = irq_data_get_irq_chip_data(d);

	writel_relaxed(d->hwirq + COMM_IRQ_START, data->intcl_reg + INTCL_CACR);
}

static int csky_irqdomain_map(struct irq_domain *d, unsigned int irq,
			      irq_hw_number_t hwirq)
{
	struct csky_irq_v2_data *data = d->host_data;

	irq_set_chip_and_handler(irq, &data->chip, handle_fasteoi_irq);
	irq_set_chip_data(irq, data);
	irq_set_noprobe(irq);
	irq_set_affinity(irq, cpumask_of(data->cpuid));

	return 0;
}

static const struct irq_domain_ops csky_irqdomain_ops = {
	.map	= csky_irqdomain_map,
	.xlate	= irq_domain_xlate_onecell,
};

static int __init
csky_intc_v2_init(struct device_node *node, struct device_node *parent)
{
	int cpuid = 0;
	struct csky_irq_v2_data *data;

	if (parent)
		return 0;

	if (INTCG_base == NULL) {
		#if defined(__CSKYABIV2__)
		INTCG_base = ioremap(mfcr("cr<31, 14>"), INTC_SIZE);
		if (INTCG_base == NULL)
		#endif
			return -EIO;

		INTCL_base = INTCG_base + INTCG_SIZE;

		writel_relaxed(BIT(0), INTCG_base + INTCG_ICTLR);
	}

	writel_relaxed(BIT(0), INTCL_base + INTCL_PICTLR);

	//cpuid = csky_of_processor_cpuid(node->parent);
	if (cpuid < 0)
		return -EIO;

	data = &per_cpu(csky_irq_v2_data, cpuid);
	snprintf(data->name, sizeof(data->name), "csky,intc-v2,cpu-%d", cpuid);
	data->cpuid = cpuid;
	data->intcl_reg = INTCL_base + (INTCL_SIZE * cpuid);
	data->chip.name = data->name;
	data->chip.irq_eoi = csky_irq_v2_eoi;
	data->chip.irq_enable = csky_irq_v2_enable;
	data->chip.irq_disable = csky_irq_v2_disable;
	data->domain = irq_domain_add_linear(node, INTC_IRQS,
					     &csky_irqdomain_ops, data);
	if (!data->domain)
		return -ENXIO;

	set_handle_irq(&csky_irq_v2_handler);

	return 0;
}
IRQCHIP_DECLARE(csky_intc_v2, "csky,intc-v2", csky_intc_v2_init);

