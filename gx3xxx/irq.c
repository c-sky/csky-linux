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

static void gx3xxx_irq_en(unsigned int irq)
{
	unsigned int mask;
	unsigned int irq_chan;

	irq_chan = irq_num[irq];
	if (irq_chan == 0xff) return;

	if (irq_chan < 32) {
		mask = 1 << irq_chan;
		__raw_writel(mask, GX3201_VA_INTC_NENSET31_00);
	} else {
		mask = 1 << (irq_chan - 32);
		__raw_writel(mask, GX3201_VA_INTC_NENSET63_32);
	}
}

static void gx3xxx_irq_dis(unsigned int irq)
{
	unsigned int mask;
	unsigned int irq_chan;

	irq_chan = irq_num[irq];
	if (irq_chan == 0xff) return;

	if (irq_chan < 32) {
		mask = 1 << irq_chan;
		__raw_writel(mask, GX3201_VA_INTC_NENCLR31_00);
	} else {
		mask = 1 << (irq_chan - 32);
		__raw_writel(mask, GX3201_VA_INTC_NENCLR63_32);
	}
}

static inline void gx3xxx_irq_ack(struct irq_data *d) {}

unsigned int gx3xxx_irq_channel_set(struct irq_data *d)
{
	unsigned int status, i;

	i = d->irq;
	irq_num[i] = i;
	status = __raw_readl(GX3201_VA_INTC_SOURCE + i - i%4) &
				((i << ((i%4)*8)) | ~(0xff << ((i%4)*8)));
	__raw_writel(status, GX3201_VA_INTC_SOURCE + i - i%4);

	gx3xxx_irq_en(d->irq);
	gx3xxx_irq_unmask(d);
	return 0;
}

void gx3xxx_irq_channel_clear(struct irq_data *d)
{
	unsigned int i, status;

	i = irq_num[d->irq];
	irq_num[d->irq] = 0xff;

	gx3xxx_irq_mask(d);
	gx3xxx_irq_dis(d->irq);

	status = __raw_readl(GX3201_VA_INTC_SOURCE + i - i%4);
	status = status | (0xff << ((i%4)*8));
	__raw_writel(status, GX3201_VA_INTC_SOURCE + i - i%4);
}

struct irq_chip gx3xxx_irq_chip = {
	.name =     "GX3XXX_IntCtrl",
	.irq_ack =      gx3xxx_irq_ack,
	.irq_mask =     gx3xxx_irq_mask,
	.irq_unmask =   gx3xxx_irq_unmask,
	.irq_startup =  gx3xxx_irq_channel_set,
	.irq_shutdown = gx3xxx_irq_channel_clear,
};

void __init gx3xxx_init_IRQ(void)
{
	unsigned int i;

	for (i = 0; i < NR_IRQS; ++i) {
		irq_set_chip_and_handler(i, &gx3xxx_irq_chip, handle_level_irq);
		irq_num[i] = 0xFF;
		if(i%4 == 0)
			__raw_writel(0xffffffff,GX3201_VA_INTC_SOURCE + i);
	}

	__raw_writel(0xffffffff, GX3201_VA_INTC_NENCLR31_00);
	__raw_writel(0xffffffff, GX3201_VA_INTC_NENCLR63_32);

	__raw_writel(0xffffffff, GX3201_VA_INTC_NMASK31_00);
	__raw_writel(0xffffffff, GX3201_VA_INTC_NMASK63_32);
}

// find first one in 64bit { hi[31:0],lo[31:0] }
// return -1 if none
// return 63 if hi[31] is one
inline int ff1_64(unsigned int hi, unsigned int lo)
{
	int result;
	__asm__ __volatile__(
		"ff1 %0"
		:"=r"(hi)
		:"r"(hi)
		:
	);

	__asm__ __volatile__(
		"ff1 %0"
		:"=r"(lo)
		:"r"(lo)
		:
	);
	if( lo != 32 )
		result = 31-lo;
	else if( hi != 32 ) result = 31-hi + 32; 
	else {
		printk("mach_get_auto_irqno error hi:%x, lo:%x.\n", hi, lo);
		result = NR_IRQS;
	}
	return result;
}

unsigned char irq_list_bak[32] = {0};
unsigned int irq_list_bak_pos = 0;

unsigned int gx3201_get_irqno(void)
{
	unsigned int nint64hi;
	unsigned int nint64lo;
	int irq_no;
	nint64lo = __raw_readl(GX3201_VA_INTC_NINT31_00);
	nint64hi = __raw_readl(GX3201_VA_INTC_NINT63_32);
	irq_no = ff1_64(nint64hi, nint64lo);

	irq_list_bak[irq_list_bak_pos] = irq_no;
	irq_list_bak_pos++;
	if (irq_list_bak_pos == 32) irq_list_bak_pos = 0;

	return irq_no;
}


