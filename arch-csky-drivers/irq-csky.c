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

static unsigned int intc_reg;

#define CK_VA_INTC_ICR		(void *)(intc_reg + 0x00)	/* Interrupt control register(High 16bits) */
#define CK_VA_INTC_ISR		(void *)(intc_reg + 0x00)	/* Interrupt status register(Low 16bits) */
#define CK_VA_INTC_NEN31_00	(void *)(intc_reg + 0x10)	/* Normal interrupt enable register Low */
#define	CK_VA_INTC_NEN63_32	(void *)(intc_reg + 0x28)	/* Normal interrupt enable register High */
#define CK_VA_INTC_IFR31_00	(void *)(intc_reg + 0x08)	/* Normal interrupt force register Low */
#define CK_VA_INTC_IFR63_32	(void *)(intc_reg + 0x20)	/* Normal interrupt force register High */
#define	CK_VA_INTC_SOURCE	(void *)(intc_reg + 0x40)	/* Proiority Level Select Registers 0 */

static void ck_irq_mask(struct irq_data *d)
{
	unsigned int temp, irq;

	irq = d->irq;

	if (irq < 32) {
		temp = readl_relaxed(CK_VA_INTC_NEN31_00);
		temp &= ~(1 << irq);
		writel_relaxed(temp, CK_VA_INTC_NEN31_00);
	} else {
		temp = readl_relaxed(CK_VA_INTC_NEN63_32);
		temp &= ~(1 << (irq -32));
		writel_relaxed(temp, CK_VA_INTC_NEN63_32);
	}
}

static void ck_irq_unmask(struct irq_data *d)
{
	unsigned int temp, irq;

	irq = d->irq;

	/* we need set IFR to pull-down the irq line */
	if (irq < 32) {
		temp = readl_relaxed(CK_VA_INTC_IFR31_00);
		temp &= ~(1 << irq);
		writel_relaxed(temp, CK_VA_INTC_IFR31_00);
	} else {
		temp = readl_relaxed(CK_VA_INTC_IFR63_32);
		temp &= ~(1 << (irq -32));
		writel_relaxed(temp, CK_VA_INTC_IFR63_32);
	}

	/* unmask the irq with enable bit */
	if (irq < 32) {
		temp = readl_relaxed(CK_VA_INTC_NEN31_00);
		temp |= 1 << irq;
		writel_relaxed(temp, CK_VA_INTC_NEN31_00);
	} else {
		temp = readl_relaxed(CK_VA_INTC_NEN63_32);
		temp |= 1 << (irq -32);
		writel_relaxed(temp, CK_VA_INTC_NEN63_32);
	}
}

static struct irq_chip ck_irq_chip = {
	.name		= "csky_intc_v1",
	.irq_mask	= ck_irq_mask,
	.irq_unmask	= ck_irq_unmask,
};

static int ck_irq_map(struct irq_domain *h, unsigned int virq,
				irq_hw_number_t hw_irq_num)
{
	irq_set_chip_and_handler(virq, &ck_irq_chip, handle_level_irq);
	return 0;
}

static const struct irq_domain_ops ck_irq_ops = {
	.map	= ck_irq_map,
	.xlate	= irq_domain_xlate_onecell,
};

static struct irq_domain *root_domain;
static void ck_irq_handler(struct pt_regs *regs)
{
	irq_hw_number_t irq = readl_relaxed(CK_VA_INTC_ISR) & 0x3f;
	handle_domain_irq(root_domain, irq, regs);
}

static int __init
intc_init(struct device_node *np, struct device_node *parent)
{
	int i;

	if (parent)
		panic("pic not a root intc\n");

	intc_reg = (unsigned int)of_iomap(np, 0);
	if (!intc_reg)
		panic("%s, of_iomap err.\n", __func__);

	csky_do_IRQ_handler = ck_irq_handler;

	writel_relaxed(0, CK_VA_INTC_NEN31_00);
	writel_relaxed(0, CK_VA_INTC_NEN63_32);

	writel_relaxed(0xc0000000, CK_VA_INTC_ICR);

	/*
	 * csky irq ctrl has 64 sources.
	 */
	#define INTC_IRQS 64
	for (i=0; i<INTC_IRQS; i=i+4)
		writel_relaxed((i+3)|((i+2)<<8)|((i+1)<<16)|(i<<24),
				CK_VA_INTC_SOURCE + i);

	root_domain = irq_domain_add_linear(np, INTC_IRQS, &ck_irq_ops, NULL);
	if (!root_domain)
		panic("root irq domain not available\n");

	return 0;
}
IRQCHIP_DECLARE(csky_intc_v1, "csky,intc-v1", intc_init);

