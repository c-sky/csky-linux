/*
 * linux/arch/csky/kernel/ints.c -- General interrupt handling code
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1999  Greg Ungerer (gerg@snapgear.com)
 * Copyright (C) 1998  D. Jeff Dionne <jeff@ArcturusNetworks.com>
 *                     Kenneth Albanowski <kjahds@kjahds.com>,
 * Copyright (C) 2000  Lineo Inc. (www.lineo.com)
 * Copyright (C) 2004  Kang Sun <sunk@vlsi.zju.edu.cn>
 * Copyright (C) 2009  Hu Junshan (junshan_hu@c-sky.com)
 *
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/list.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/rtc.h>
#include <linux/irqchip.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/traps.h>
#include <asm/page.h>
#include <asm/machdep.h>

/* The number of spurious interrupts */
volatile unsigned int num_spurious;

extern e_vector vec_base;
void set_evector(int vecnum, void (*handler)(void))
{
	e_vector *_ramvec = &vec_base;
	if (vecnum >= 0 && vecnum <= 255)
		_ramvec[vecnum] = handler;
}

unsigned long irq_err_count;

int arch_show_interrupts(struct seq_file *p, int prec)
{
	seq_printf(p, "%*s: %10lu\n", prec, "ERR", irq_err_count);
	return 0;
}

/* Handle bad interrupts */
static struct irq_desc bad_irq_desc = {
	.handle_irq = handle_bad_irq,
	.lock = __RAW_SPIN_LOCK_UNLOCKED(bad_irq_desc.lock),
};

/*
 * do_IRQ handles all hardware IRQ's.  Decoded IRQs should not
 * come via this function.  Instead, they should provide their
 * own 'handler'
 */
asmlinkage void csky_do_IRQ(unsigned int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();

	/*
	 * Some hardware gives randomly wrong interrupts.  Rather
	 * than crashing, do something sensible.
	 */
	if (irq >= NR_IRQS)
		handle_bad_irq(&bad_irq_desc);
	else
		generic_handle_irq(irq);

	irq_exit();
	set_irq_regs(old_regs);
}

asmlinkage void  csky_do_auto_IRQ(struct pt_regs *regs)
{
	csky_do_IRQ(mach_get_auto_irqno(), regs);
}

/*
 * void init_IRQ(void)
 *
 * Parameters:	None
 *
 * Returns:	Nothing
 *
 * This function should be called during kernel startup to initialize
 * the IRQ handling routines.
 */

void __init init_IRQ(void)
{
	int i;

	for (i = 0; i < NR_IRQS; i++)
		irq_set_noprobe(i);

	irqchip_init();
}

