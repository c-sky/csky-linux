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

#define INTC_NINT31_00		0x00
#define INTC_NINT63_32		0x04
#define INTC_NEN31_00		0x40
#define INTC_NEN63_32		0x44
#define INTC_NENSET31_00	0x20
#define INTC_NENSET63_32	0x24
#define INTC_NENCLR31_00	0x30
#define INTC_NENCLR63_32	0x34
#define INTC_NMASK31_00		0x50
#define INTC_NMASK63_32		0x54
#define INTC_SOURCE		0x60

#define INTC_IRQS		64

static struct irq_domain *root_domain;

static void nc_irq_handler(struct pt_regs *regs)
{
	u32 status, irq;

	do {
		status = readl_relaxed(reg_base + INTC_NINT31_00);
		if (status) {
			irq = __ffs(status);
		} else {
			status = readl_relaxed(reg_base + INTC_NINT63_32);
			if (status)
				irq = __ffs(status) + 32;
			else
				return;
		}
		handle_domain_irq(root_domain, irq, regs);
	} while(1);
}

static void __init nc_set_gc(void __iomem *reg_base, u32 irq_base,
			     u32 en_reg, u32 dis_reg)
{
	struct irq_chip_generic *gc;

	gc = irq_get_domain_generic_chip(root_domain, irq_base);
	gc->reg_base = reg_base;
	gc->chip_types[0].regs.enable = en_reg;
	gc->chip_types[0].regs.disable = dis_reg;
	gc->chip_types[0].chip.irq_mask = irq_gc_mask_disable_reg;
	gc->chip_types[0].chip.irq_unmask = irq_gc_unmask_enable_reg;
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
		writel_relaxed(expand_byte_to_word(i) + 0x03020100,
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

	setup_irq_channel(reg_base);

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

	nc_set_gc(reg_base, 0,  INTC_NENSET31_00, INTC_NENCLR31_00);
	nc_set_gc(reg_base, 32, INTC_NENSET63_32, INTC_NENCLR63_32);

	set_handle_irq(nc_irq_handler);

	return 0;
}

IRQCHIP_DECLARE(nationalchip_intc_v1_ave, "nationalchip,intc-v1,ave", intc_init);

