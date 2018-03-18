// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip.h>

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
	unsigned long irq, psr;

	asm volatile("mfcr %0, psr":"=r"(psr));

	irq = (psr >> 16) & 0xff;

	if (irq == 10)
		irq = csky_get_auto_irqno();
	else
		irq -= 32;

	csky_do_IRQ(irq, regs);
}

void __init init_IRQ(void)
{
	irqchip_init();
}

