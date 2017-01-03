#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqchip.h>

unsigned int (*csky_get_auto_irqno) (void) = NULL;

int arch_show_interrupts(struct seq_file *p, int prec)
{
	return 0;
}

asmlinkage void csky_do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();
	generic_handle_irq(irq);
	irq_exit();

	set_irq_regs(old_regs);
}

asmlinkage void csky_do_auto_IRQ(struct pt_regs *regs)
{
	csky_do_IRQ(csky_get_auto_irqno(), regs);
}

void __init init_IRQ(void)
{
	irqchip_init();
}

