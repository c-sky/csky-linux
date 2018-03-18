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
		temp = __raw_readl(CK_VA_INTC_NEN31_00);
		temp &= ~(1 << irq);
		__raw_writel(temp, CK_VA_INTC_NEN31_00);
	} else {
		temp = __raw_readl(CK_VA_INTC_NEN63_32);
		temp &= ~(1 << (irq -32));
		__raw_writel(temp, CK_VA_INTC_NEN63_32);
	}
}

static void ck_irq_unmask(struct irq_data *d)
{
	unsigned int temp, irq;

	irq = d->irq;

	/* set IFR to support rising edge triggering */
	if (irq < 32) {
		temp = __raw_readl(CK_VA_INTC_IFR31_00);
		temp &= ~(1 << irq);
		__raw_writel(temp, CK_VA_INTC_IFR31_00);
	} else {
		temp = __raw_readl(CK_VA_INTC_IFR63_32);
		temp &= ~(1 << (irq -32));
		__raw_writel(temp, CK_VA_INTC_IFR63_32);
	}

	if (irq < 32) {
		temp = __raw_readl(CK_VA_INTC_NEN31_00);
		temp |= 1 << irq;
		__raw_writel(temp, CK_VA_INTC_NEN31_00);
	} else {
		temp = __raw_readl(CK_VA_INTC_NEN63_32);
		temp |= 1 << (irq -32);
		__raw_writel(temp, CK_VA_INTC_NEN63_32);
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

static unsigned int ck_get_irqno(void)
{
	unsigned int temp;
	temp = __raw_readl(CK_VA_INTC_ISR);
	return temp & 0x3f;
};

static int __init
__intc_init(struct device_node *np, struct device_node *parent, bool ave)
{
	struct irq_domain *root_domain;
	int i;

	csky_get_auto_irqno = ck_get_irqno;

	if (parent)
		panic("pic not a root intc\n");

	intc_reg = (unsigned int)of_iomap(np, 0);
	if (!intc_reg)
		panic("%s, of_iomap err.\n", __func__);

	__raw_writel(0, CK_VA_INTC_NEN31_00);
	__raw_writel(0,	CK_VA_INTC_NEN63_32);

	if (ave == true)
		__raw_writel( 0xc0000000, CK_VA_INTC_ICR);
	else
		__raw_writel( 0x0, CK_VA_INTC_ICR);
	/*
	 * csky irq ctrl has 64 sources.
	 */
	#define INTC_IRQS 64
	for (i=0; i<INTC_IRQS; i=i+4)
		__raw_writel((i+3)|((i+2)<<8)|((i+1)<<16)|(i<<24),
				CK_VA_INTC_SOURCE + i);

	root_domain = irq_domain_add_legacy(np, INTC_IRQS, 0, 0, &ck_irq_ops, NULL);
	if (!root_domain)
		panic("root irq domain not available\n");

	irq_set_default_host(root_domain);

	return 0;
}

static int __init
intc_init(struct device_node *np, struct device_node *parent)
{

	return __intc_init(np, parent, false);
}
IRQCHIP_DECLARE(csky_intc_v1, "csky,intc-v1", intc_init);

/*
 * use auto vector exceptions 10 for interrupt.
 */
static int __init
intc_init_ave(struct device_node *np, struct device_node *parent)
{
	return __intc_init(np, parent, true);
}
IRQCHIP_DECLARE(csky_intc_v1_ave, "csky,intc-v1,ave", intc_init_ave);

