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

#ifdef CONFIG_CSKY_VECIRQ_LEGENCY
#include <asm/reg_ops.h>
#endif

static void __iomem *reg_base;

#define INTC_ICR	0x00
#define INTC_ISR	0x00
#define INTC_NEN31_00	0x10
#define INTC_NEN63_32	0x28
#define INTC_IFR31_00	0x08
#define INTC_IFR63_32	0x20
#define INTC_SOURCE	0x40

#define INTC_IRQS	64

#define INTC_ICR_AVE	BIT(31)

#define VEC_IRQ_BASE	32

static struct irq_domain *root_domain;

static void __init ck_set_gc(void __iomem *reg_base, u32 irq_base,
			     u32 mask_reg)
{
	struct irq_chip_generic *gc;

	gc = irq_get_domain_generic_chip(root_domain, irq_base);
	gc->reg_base = reg_base;
	gc->chip_types[0].regs.mask = mask_reg;
	gc->chip_types[0].chip.irq_mask = irq_gc_mask_clr_bit;
	gc->chip_types[0].chip.irq_unmask = irq_gc_mask_set_bit;
}

static struct irq_domain *root_domain;
static void ck_irq_handler(struct pt_regs *regs)
{
#ifdef CONFIG_CSKY_VECIRQ_LEGENCY
	irq_hw_number_t irq = ((mfcr("psr") >> 16) & 0xff) - VEC_IRQ_BASE;
#else
	irq_hw_number_t irq = readl_relaxed(reg_base + INTC_ISR) & 0x3f;
#endif
	handle_domain_irq(root_domain, irq, regs);
}

#define expand_byte_to_word(i) (i|(i<<8)|(i<<16)|(i<<24))
static inline void setup_irq_channel(void __iomem *reg_base)
{
	int i;

	/*
	 * There are 64 irq nums and irq-channels and one byte per channel.
	 * Setup every channel with the same hwirq num.
	 */
	for (i = 0; i < INTC_IRQS; i += 4) {
		writel_relaxed(expand_byte_to_word(i) + 0x00010203,
			       reg_base + INTC_SOURCE + i);
	}
}

static int __init
csky_intc_v1_init(struct device_node *node, struct device_node *parent)
{
	u32 clr = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	int ret;

	if (parent) {
		pr_err("C-SKY Intc not a root irq controller\n");
		return -EINVAL;
	}

	reg_base = of_iomap(node, 0);
	if (!reg_base) {
		pr_err("C-SKY Intc unable to map: %p.\n", node);
		return -EINVAL;
	}

	writel_relaxed(0, reg_base + INTC_NEN31_00);
	writel_relaxed(0, reg_base + INTC_NEN63_32);

#ifndef CONFIG_CSKY_VECIRQ_LEGENCY
	writel_relaxed(INTC_ICR_AVE, reg_base + INTC_ICR);
#else
	writel_relaxed(0, reg_base + INTC_ICR);
#endif

	setup_irq_channel(reg_base);

	root_domain = irq_domain_add_linear(node, INTC_IRQS, &irq_generic_chip_ops, NULL);
	if (!root_domain) {
		pr_err("C-SKY Intc irq_domain_add failed.\n");
		return -ENOMEM;
	}

	ret = irq_alloc_domain_generic_chips(root_domain, 32, 1,
					     "csky_intc_v1", handle_level_irq,
					     clr, 0, 0);
	if (ret) {
		pr_err("C-SKY Intc irq_alloc_gc failed.\n");
		return -ENOMEM;
	}

	ck_set_gc(reg_base, 0,  INTC_NEN31_00);
	ck_set_gc(reg_base, 32, INTC_NEN63_32);

	set_handle_irq(ck_irq_handler);

	return 0;
}
IRQCHIP_DECLARE(csky_intc_v1, "csky,intc-v1", csky_intc_v1_init);

