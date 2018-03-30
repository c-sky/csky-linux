// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip.h>
#include <asm/traps.h>

unsigned int (*csky_get_auto_irqno) (void) = NULL;

void csky_do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();
	generic_handle_irq(irq);
	irq_exit();

	set_irq_regs(old_regs);
}

asmlinkage void csky_do_auto_IRQ(struct pt_regs *regs)
{
	unsigned long irq;

	/*
	 * PSR format:
	 * |31  24|23        16|15  0|
	 *          Vector Num
	 */
	irq = (mfcr("psr") >> 16) & 0xff;

	/*
	 * Vector 0  - 31 is for exceptions
	 *
	 * Vector 31 - xx is for vector-irqs, we can get irq from psr.
	 * Or
	 * Vector 10 is the auto vector, we need get irq from irq-ctrl.
	 */
	if (irq == VEC_AUTOVEC)
		irq = csky_get_auto_irqno();
	else
		irq -= VEC_IRQ_BASE;

	csky_do_IRQ(irq, regs);
}

void __init init_IRQ(void)
{
	irqchip_init();
}

