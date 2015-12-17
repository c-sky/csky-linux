#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/map.h>
#include <mach/irqs.h>

static unsigned int irq_num[NR_IRQS];

static void gx3xxx_irq_mask(struct irq_data *d)
{
	unsigned int mask;
	unsigned int irq_chan;

	irq_chan = irq_num[d->irq];
	if (irq_chan == 0xff) return;

	if (irq_chan < 32) {
		mask = __raw_readl(GX3201_VA_INTC_NMASK31_00);
		mask |= 1 << irq_chan;
		__raw_writel(mask, GX3201_VA_INTC_NMASK31_00);
	} else {
		mask = __raw_readl(GX3201_VA_INTC_NMASK63_32);
		mask |= 1 << (irq_chan - 32);
		__raw_writel(mask, GX3201_VA_INTC_NMASK63_32);
	}
}

static void gx3xxx_irq_unmask(struct irq_data *d)
{
	unsigned int mask;
	unsigned int irq_chan;

	irq_chan = irq_num[d->irq];
	if (irq_chan == 0xff) return;

	if (irq_chan < 32) {
		mask = __raw_readl(GX3201_VA_INTC_NMASK31_00);
		mask &= ~( 1 << irq_chan);
		__raw_writel(mask, GX3201_VA_INTC_NMASK31_00);
	} else {
		mask = __raw_readl( GX3201_VA_INTC_NMASK63_32);
		mask &= ~(1 << (irq_chan - 32));
		__raw_writel(mask, GX3201_VA_INTC_NMASK63_32);
	}
}

struct irq_chip gx3xxx_irq_chip = {
        .name           = "GX3XXX_IntCtrl",
        .irq_mask           = gx3xxx_irq_mask,
        .irq_unmask         = gx3xxx_irq_unmask,
};

#if 0
void gx3xxx_irq_channel_set(unsigned int irq)
{
	unsigned int i, status, status2;

	for (i = 0; i < NR_IRQS; i++) {
		status = __raw_readl(GX3201_VA_INTC_SOURCE + i - i%4);
		status2 = (status >> ((i%4)*8)) & 0xff;
		if (status2 == 0xff) {
			status2 = irq;
			irq_num[irq] = i;
			status2 = (status2 << ((i%4)*8)) | ~(0xff << ((i%4)*8));
			status = status & status2;
			printk("gx3xxx_irq %d at channel %d, base_va: %x.\n", irq, i, GX3201_VA_INTC_SOURCE);
			__raw_writel(status, GX3201_VA_INTC_SOURCE + i - i%4);
			break;
		}
	}
}
#else
void gx3xxx_irq_channel_set(unsigned int i)
{
	unsigned int status;

	irq_num[i] = i;
	status = __raw_readl(GX3201_VA_INTC_SOURCE + i - i%4) &
				((i << ((i%4)*8)) | ~(0xff << ((i%4)*8)));
	__raw_writel(status, GX3201_VA_INTC_SOURCE + i - i%4);
}
#endif

void gx3xxx_irq_channel_clear(unsigned int irq)
{
	unsigned int i, status;

	i = irq_num[irq];
	irq_num[irq] = 0xff;

	status = __raw_readl(GX3201_VA_INTC_SOURCE + i - i%4);
	status = status | (0xff << ((i%4)*8));
	__raw_writel(status, GX3201_VA_INTC_SOURCE + i - i%4);
}

void __init gx3xxx_init_IRQ(void)
{
	unsigned int i;

	for (i = 0; i < NR_IRQS; ++i) 
	{
//	set_irq_chip(i,&gx3xxx_irq_chip);
		irq_set_chip_and_handler(i, &gx3xxx_irq_chip, handle_level_irq);
		irq_num[i] = 0xFF;
		if(i%4 == 0)
			__raw_writel(0xFFFFFFFF,GX3201_VA_INTC_SOURCE + i);
	}

	__raw_writel(0xFFFFFFFF,GX3201_VA_INTC_NEN31_00);
	__raw_writel(0xFFFFFFFF,GX3201_VA_INTC_NEN63_32);

	__raw_writel(0xFFFFFFFF,GX3201_VA_INTC_NMASK31_00);
	__raw_writel(0xFFFFFFFF,GX3201_VA_INTC_NMASK63_32);
}

unsigned int gx3201_get_irqno(void)
{
	unsigned int i;
	unsigned long long nint64;

	nint64 = __raw_readl(GX3201_VA_INTC_NINT63_32);
	nint64 = nint64 << 32;
	nint64 = nint64 | __raw_readl(GX3201_VA_INTC_NINT31_00);

	for (i = 0; i < NR_IRQS; i++) {
		if ((nint64 >> i) & 1) {
#if 0
			printk("NINT63_00:0x%x%x\n",
			__raw_readl(GX3201_VA_INTC_NINT63_32),
			__raw_readl(GX3201_VA_INTC_NINT31_00));
			printk("interrupt number is:%d\n",i);
#endif
			return i;
		}
	}

	return -1;
}

