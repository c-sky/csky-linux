// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou NationalChip Science & Technology Co.,Ltd.
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

static void __iomem *reg_base;

#define INTC_PEN31_00		0x00
#define INTC_PEN63_32		0x04
#define INTC_NEN31_00		0x40
#define INTC_NEN63_32		0x44
#define INTC_NMASK31_00		0x50
#define INTC_NMASK63_32		0x54
#define INTC_SOURCE		0x60

#define INTC_IRQS		64

static struct irq_domain *root_domain;

static inline int handle_irq_perbit(struct pt_regs *regs, u32 hwirq, u32 irq_base)
{
	u32 irq;

	if (hwirq == 0) return 0;

	while (hwirq) {
		irq = __ffs(hwirq);
		hwirq &= ~BIT(irq);
		handle_domain_irq(root_domain, irq_base + irq, regs);
	}

	return 1;
}

static void ck_irq_handler(struct pt_regs *regs)
{
	int ret;

	do {
		ret  = handle_irq_perbit(regs ,readl_relaxed(reg_base + INTC_PEN31_00), 0);
		ret |= handle_irq_perbit(regs ,readl_relaxed(reg_base + INTC_PEN63_32), 32);
	} while(ret);
}

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

#define expand_byte_to_word(i) (i|(i<<8)|(i<<16)|(i<<24))
static inline void setup_irq_channel(void __iomem *reg_base, u32 channel_magic)
{
	int i;

	/*
	 * There are 64 irq nums and irq-channels and one byte per channel.
	 * Setup every channel with the same hwirq num.
	 */
	for (i = 0; i < INTC_IRQS; i += 4) {
		writel_relaxed(expand_byte_to_word(i) + channel_magic,
			       reg_base + INTC_SOURCE + i);
	}
}

static int __init
intc_init(struct device_node *node, struct device_node *parent)
{
	u32 clr = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	int ret;

	if (parent) {
		pr_err("Nationalchip gx6605s Intc not a root irq controller\n");
		return -EINVAL;
	}

	reg_base = of_iomap(node, 0);
	if (!reg_base) {
		pr_err("Nationalchip gx6605s Intc unable to map: %p.\n", node);
		return -EINVAL;
	}

	/* Initial enable reg to disable all interrupts */
	writel_relaxed(0x0, reg_base + INTC_NEN31_00);
	writel_relaxed(0x0, reg_base + INTC_NEN63_32);

	/* Initial mask reg with all unmasked, becasue we only use enalbe reg */
	writel_relaxed(0x0, reg_base + INTC_NMASK31_00);
	writel_relaxed(0x0, reg_base + INTC_NMASK63_32);

	setup_irq_channel(reg_base, 0x03020100);

	root_domain = irq_domain_add_linear(node, INTC_IRQS, &irq_generic_chip_ops, NULL);
	if (!root_domain) {
		pr_err("Nationalchip gx6605s Intc irq_domain_add failed.\n");
		return -ENOMEM;
	}

	ret = irq_alloc_domain_generic_chips(root_domain, 32, 1,
					     "gx6605s_irq", handle_level_irq,
					     clr, 0, 0);
	if (ret) {
		pr_err("Nationalchip gx6605s Intc irq_alloc_gc failed.\n");
		return -ENOMEM;
	}

	ck_set_gc(reg_base, 0,  INTC_NEN31_00);
	ck_set_gc(reg_base, 32, INTC_NEN63_32);

	set_handle_irq(ck_irq_handler);

	return 0;
}

IRQCHIP_DECLARE(nationalchip_intc_v1_ave, "nationalchip,intc-v1,ave", intc_init);

